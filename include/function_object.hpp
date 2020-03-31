#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "exception.hpp"
#include "serialize.hpp"
#include "utils.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

using Buffers = vector<string>;

class BoxedValue {  // move value to stack TODO need optimization to reduce
                    // copy. Need to consider how the serialization is done
                    // TODO test for all possible type
                    // TODO type consistancy check
    shared_ptr<void> v_ptr;

   protected:
    BoxedValue() = delete;

   public:
    BoxedValue(const BoxedValue &) = default;
    BoxedValue(BoxedValue &&bv) = default;

    template <class T,
              class = typename enable_if<!is_pointer<T>::value &&
                                         is_copy_constructible<T>::value>::type>
    explicit BoxedValue(
        const T &t) {  // copy to heap , this is very dangerous to match T ...
        auto sp = make_shared<T>(t);
        v_ptr = std::static_pointer_cast<void>(sp);
    }
    template <
        class T,
        class = typename enable_if<!is_lvalue_reference<T>::value>::
            type>  // must specify or it will be forwarding reference
                   // why is !is_lvalue_reference, see
                   // https://stackoverflow.com/questions/53758796/why-does-stdis-rvalue-reference-not-do-what-it-is-advertised-to-do
    BoxedValue(T &&t) {
        auto sp = make_shared<T>(forward<T>(t));
        v_ptr = std::static_pointer_cast<void>(sp);
    }

    template <class T>
    BoxedValue(unique_ptr<T> &&p) {
        shared_ptr<T> sp{move(p)};
        v_ptr = std::static_pointer_cast<void>(sp);
    }

    template <class T>
    BoxedValue(shared_ptr<T> &sp) {
        v_ptr = std::static_pointer_cast<void>(sp);
    }
    template <class T>
    static shared_ptr<T> boxCast(BoxedValue &b) {
        return std::static_pointer_cast<T>(b.v_ptr);
    }
};

namespace exception {
class ParameterError : public std::range_error {
   public:
    ParameterError(const std::string &what) : range_error(what) {}
};
}  // namespace exception

class FunctionObjectBase {
   public:
    // virtual BoxedValue operator()(BoxedValue args...) =0;
    virtual BoxedValue operator()(vector<BoxedValue> &args) = 0;

    virtual vector<BoxedValue> deSerializeArguments(const Buffers &) = 0;

   protected:
    virtual ~FunctionObjectBase() {}
};

template <class Ret, class... ArgTypes>
class FunctionSingnature {
   public:
    static vector<BoxedValue> deSerializeArguments(const Buffers &buffers) {
        auto bvs = vector<BoxedValue>{};
        _deSerializeArguments<0, ArgTypes...>(bvs, buffers);
        return bvs;
    }

    static function<Ret()> bindArguments(function<Ret(ArgTypes...)> &f,
                                         vector<BoxedValue> &bvs) {
        return _bindArguments<0, ArgTypes...>(f, bvs);
    }

   private:
    template <int index, class Arg1, class... ArgTs>  // TODO: by forwording?
    static void _deSerializeArguments(vector<BoxedValue> &bvs,
                                      const Buffers &buffers) {
        if (index + 1 > buffers.size()) {
            throw thallium::exception::ParameterError(format(
                "Function arguments arity doesnt match: given {}, need {}",
                buffers.size(), 1 + index + sizeof...(ArgTs)));
        }
        bvs.push_back(Serializer::deSerialize<Arg1>(buffers[index]));
        _deSerializeArguments<index + 1, ArgTs...>(bvs, buffers);
    }
    template <int index>
    static void _deSerializeArguments(vector<BoxedValue> &bvs,
                                      const Buffers &buffers) {
        if (bvs.size() != buffers.size()) {
            throw thallium::exception::ParameterError(format(
                "Function arguments arity doesnt match: given {}, need {}",
                buffers.size(), index));
        }
    }

    template <int index, class Arg1, class... ArgTs>
    static function<Ret()> _bindArguments(function<Ret(Arg1, ArgTs...)> &f,
                                          vector<BoxedValue> &bvs) {
        if (index == bvs.size()) {
            auto e = thallium::exception::ParameterError(format(
                "Function arguments arity doesnt match: given {}, need {}",
                bvs.size(), 1 + index + sizeof...(ArgTs)));
            TI_RAISE(e);
        }
        auto a1 = BoxedValue::boxCast<Arg1>(bvs[index]);

        function<Ret(ArgTs...)> _f = [a1, f](ArgTs... args) {
            return f(*a1, args...);
        };
        return _bindArguments<index + 1, ArgTs...>(_f, bvs);
    }

    template <int index>
    static function<Ret()> _bindArguments(function<Ret()> &f,
                                          vector<BoxedValue> &bvs) {
        if (index != bvs.size()) {
            auto e = thallium::exception::ParameterError(format(
                "Function arguments arity doesnt match: given {}, need {}",
                bvs.size(), index));
            TI_RAISE(e);
        }
        return f;
    }
};

template <class Ret, class... ArgTypes>
class FunctionObjectImpl : public FunctionObjectBase {
    using FuncSignature = FunctionSingnature<Ret, ArgTypes...>;
    function<Ret(ArgTypes...)> f;

   public:
    FunctionObjectImpl(Ret(func)(ArgTypes...))
        : FunctionObjectBase(), f(func) {}

    FunctionObjectImpl(const function<Ret(ArgTypes...)> &func)
        : FunctionObjectBase(), f(func) {}

    vector<BoxedValue> deSerializeArguments(const Buffers &bs) override {
        return FuncSignature::deSerializeArguments(bs);
    }

    BoxedValue operator()(vector<BoxedValue> &args) override {
        auto f_bd = FuncSignature::bindArguments(f, args);
        return f_bd();
    }
};

template <class Ret, class... ArgTypes>
class VoidFunctionObject : public FunctionObjectImpl<Ret, ArgTypes...> {
    static_assert(is_same<Ret, void>::value,
                  "Void function's return type must be void!");
};

template <class Ret, class... ArgTypes>
using FunctionObject = FunctionObjectImpl<Ret, ArgTypes...>;

template <class Ret, class... ArgTypes>
FunctionObjectImpl<Ret, ArgTypes...> make_function_object(
    Ret(func)(ArgTypes...)) {
    return FunctionObjectImpl<Ret, ArgTypes...>(func);
}
template <class Ret, class... ArgTypes>
FunctionObjectImpl<Ret, ArgTypes...> make_function_object(
    const function<Ret(ArgTypes...)> &func) {
    return FunctionObjectImpl<Ret, ArgTypes...>(func);
}
template <class... ArgTypes>
FunctionObjectImpl<void, ArgTypes...> make_void_function_object(
    void(func)(ArgTypes...)) {
    return VoidFunctionObject<void, ArgTypes...>(func);
}
template <class... ArgTypes>
FunctionObjectImpl<void, ArgTypes...> make_void_function_object(
    const function<void(ArgTypes...)> &func) {
    return VoidFunctionObject<void, ArgTypes...>(func);
}

class FuncManager : public Singleton<FuncManager> {
    friend class Singleton<FuncManager>;
    unordered_map<string, shared_ptr<FunctionObjectBase>> f_table;

   public:
    FuncManager() {}

    void addFunc(const string &f_id,
                 const shared_ptr<FunctionObjectBase> &f_ptr) {
      if(f_table.count(f_id) != 0)
        TI_RAISE(std::logic_error(thallium::format("The function: {} has been registered!", f_id)));
      f_table.insert({f_id, f_ptr});
    }

    shared_ptr<FunctionObjectBase> getFuncObj(const string &f_id) {
        return f_table[f_id];
    }
};

template <class Ret, class... ArgTypes>
void register_func(Ret(func)(
    ArgTypes...)) {  // redundant codes, refectoring after future changes.
    auto _p = make_shared<FunctionObject<Ret,ArgTypes...>>(func);
    auto p = dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get()->addFunc(f_id, p);
}

template <class Ret, class... ArgTypes>
void register_func(function<Ret(ArgTypes...)> &func) {
    auto _p = make_shared<FunctionObject<Ret,ArgTypes...>>(func);
    auto p = dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get()->addFunc(f_id, p);
}

template <class... ArgTypes>
void register_void_func(void(func)(ArgTypes...)) {
    auto _p = make_shared<VoidFunctionObject<ArgTypes...>>(func);
    auto p = dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get()->addFunc(f_id, p);
}

template <class... ArgTypes>
void register_void_func(function<void(ArgTypes...)> &func) {
    auto _p = make_shared<VoidFunctionObject<ArgTypes...>>(func);
    auto p = dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get()->addFunc(f_id, p);
}

shared_ptr<FunctionObjectBase> getFunctionObject(const string &f_id){
  return FuncManager::get()->getFuncObj(f_id);
}

_THALLIUM_END_NAMESPACE
