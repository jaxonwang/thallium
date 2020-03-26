#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "common.hpp"
#include "exception.hpp"
#include "serialize.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

using Buffers = vector<string>;

class BoxedValue{ //move value to stack TODO need optimization to reduce copy. Need to consider how the serialization is done
    shared_ptr <void> v_ptr;
    
    protected:
    BoxedValue() =delete;
    // BoxedValue(const BoxedValue &b) =delete;

    public:
    BoxedValue(BoxedValue &&a){
        v_ptr = move(a.v_ptr);
    }
    template<class T>
    BoxedValue(T &t){
        auto sp =  make_shared<T>();
        *sp = t;
        v_ptr = std::static_pointer_cast<void>(sp);
    }
    template<class T>
    BoxedValue(const T &t){
        auto sp =  make_shared<T>();
        *sp = t;
        v_ptr = std::static_pointer_cast<void>(sp);
    }
    template<class T>
    BoxedValue(T &&t){
        auto sp =  make_shared<T>();
        *sp = move(t);
        v_ptr = std::static_pointer_cast<void>(sp);
    }

    template<class T>
    BoxedValue(unique_ptr<T> &&p){
        shared_ptr<T> sp{move(p)};
        v_ptr = std::static_pointer_cast<void>(sp);
    }

    template<class T>
    BoxedValue(shared_ptr<T> &sp){
        v_ptr = std::static_pointer_cast<void>(sp);
    }
    template<class T>
    static shared_ptr<T> boxCast(BoxedValue &b){
        return std::static_pointer_cast<T>(b.v_ptr);
    }
};

namespace exception {
class ParameterError : public std::range_error {
   public:
    ParameterError(const std::string &what) : range_error(what) {}
};
}  // namespace exception

class FunctionObject {
   public:
    // virtual BoxedValue operator()(BoxedValue args...) =0;
    virtual BoxedValue operator()(vector<BoxedValue> &args) = 0;

    virtual vector<BoxedValue> deSerializeArguments(const Buffers &) = 0;
    protected:
    virtual ~FunctionObject(){}
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
    template <int index, class Arg1, class... ArgTs>
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
class FunctionObjectImpl : public FunctionObject {
    using FuncSignature = FunctionSingnature<Ret, ArgTypes...>;
    function<Ret(ArgTypes...)> f;

   public:
    FunctionObjectImpl(Ret(func)(ArgTypes...)) : FunctionObject(), f(func) {}

    FunctionObjectImpl(const function<Ret(ArgTypes...)> &func)
        : FunctionObject(), f(func) {}

    vector<BoxedValue> deSerializeArguments(const Buffers &bs) override {
        return FuncSignature::deSerializeArguments(bs);
    }

    BoxedValue operator()(vector<BoxedValue> &args) override {
        auto f_bd = FuncSignature::bindArguments(f, args);
        return f_bd();
    }
};

template <class Ret, class... ArgTypes>
FunctionObjectImpl<Ret, ArgTypes...> make_function_object(
    Ret(func)(ArgTypes...)) {
    return FunctionObjectImpl<Ret, ArgTypes...>(func);
}
template <class Ret, class... ArgTypes>
FunctionObjectImpl<Ret, ArgTypes...> make_function_object(
    const function<Ret(ArgTypes...)> func) {
    return FunctionObjectImpl<Ret, ArgTypes...>(func);
}

_THALLIUM_END_NAMESPACE
