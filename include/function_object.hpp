#ifndef _THALLIUM_FUNCOBJ_HPP
#define _THALLIUM_FUNCOBJ_HPP

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

using Buffers = std::vector<std::string>;
typedef unsigned int FuncId;

class BoxedValue {  // move value to stack TODO need optimization to reduce
                    // copy. Need to consider how the serialization is done
                    // TODO test for all possible type
                    // TODO type consistancy check
    std::shared_ptr<void> v_ptr;

  protected:
    BoxedValue() = delete;

  public:
    BoxedValue(const BoxedValue &) = default;
    BoxedValue(BoxedValue &&bv) = default;

    template <class T,
              class = typename std::enable_if<
                  !std::is_pointer<T>::value &&
                  std::is_copy_constructible<T>::value>::type>
    explicit BoxedValue(
        const T &t) {  // copy to heap , this is very dangerous to match T ...
        auto sp = std::make_shared<T>(t);
        v_ptr = std::static_pointer_cast<void>(sp);
    }
    template <
        class T,
        class = typename std::enable_if<!std::is_lvalue_reference<T>::value>::
            type>  // must specify or it will be forwarding reference
                   // why is !is_lvalue_reference, see
                   // https://stackoverflow.com/questions/53758796/why-does-stdis-rvalue-reference-not-do-what-it-is-advertised-to-do
    BoxedValue(T &&t) {
        auto sp = std::make_shared<T>(std::forward<T>(t));
        v_ptr = std::static_pointer_cast<void>(sp);
    }

    template <class T>
    BoxedValue(std::unique_ptr<T> &&p) {
        std::shared_ptr<T> sp{move(p)};
        v_ptr = std::static_pointer_cast<void>(sp);
    }

    template <class T>
    BoxedValue(std::shared_ptr<T> &sp) {
        v_ptr = std::static_pointer_cast<void>(sp);
    }
    template <class T, class T_d = typename std::remove_reference<T>::type>
    static std::shared_ptr<T_d> boxCast(BoxedValue &b) {
        return std::static_pointer_cast<T_d>(b.v_ptr);
    }
};

struct FuncIdGen {  // TODO need thread safe?
    static FuncId current;
    static std::unordered_map<std::string, FuncId> id_lookup;
    static FuncId genId() { return current++; }
    static FuncId getIdfromMangled(const std::string &s);
};

template <class T>
const std::string get_mangled(const T t) {
    return typeid(t).name();
}

template <class Rt, class... Argtypes>
FuncId function_id(Rt(f)(Argtypes...)) {
    return FuncIdGen::getIdfromMangled(get_mangled(f));
}

template <class Rt, class... Argtypes>
FuncId function_id(std::function<Rt(Argtypes...)> &f) {
    return FuncIdGen::getIdfromMangled(get_mangled(f));
}

namespace exception {
class ParameterError : public std::range_error {
  public:
    ParameterError(const std::string &what) : range_error(what) {}
};
}  // namespace exception

class FunctionObjectBase {
  public:
    // virtual BoxedValue operator()(BoxedValue args...) =0;
    virtual BoxedValue operator()(std::vector<BoxedValue> &args) = 0;

    virtual std::vector<BoxedValue> deSerializeArguments(const Buffers &) = 0;

  protected:
    virtual ~FunctionObjectBase() {}
};

template <class T>
class FunctionSingnature;

extern const std::string arity_error_str;

template <class Ret, class... ArgTypes>
class FunctionSingnature<Ret(ArgTypes...)> {
  public:
    static std::vector<BoxedValue> deSerializeArguments(
        const Buffers &buffers) {
        auto bvs = std::vector<BoxedValue>{};
        _deSerializeArguments<0, ArgTypes...>(bvs, buffers);
        return bvs;
    }

    static std::function<Ret()> bindArguments(
        std::function<Ret(ArgTypes...)> &f, std::vector<BoxedValue> &bvs) {
        return _bindArguments<0>(f, bvs);
    }

  private:
    template <int index, class Arg1, class... ArgTs>
    static void _deSerializeArguments(std::vector<BoxedValue> &bvs,
                                      const Buffers &buffers) {
        if (index + 1 > buffers.size()) {
            throw thallium::exception::ParameterError(format(arity_error_str,
                buffers.size(), 1 + index + sizeof...(ArgTs)));
        }
        bvs.push_back(
            Serializer::create_from_string<tl_remove_cvref_t<Arg1>>(buffers[index]));
        _deSerializeArguments<index + 1, ArgTs...>(bvs, buffers);
    }
    template <int index>
    static void _deSerializeArguments(std::vector<BoxedValue> &bvs,
                                      const Buffers &buffers) {
        if (bvs.size() != buffers.size()) {
            throw thallium::exception::ParameterError(format(arity_error_str,
                buffers.size(), index));
        }
    }

    template <int index, class Arg1, class... ArgTs>
    static std::function<Ret()> _bindArguments(
        std::function<Ret(Arg1, ArgTs...)> &f, std::vector<BoxedValue> &bvs) {
        if (index == bvs.size()) {
            auto e = thallium::exception::ParameterError(format(arity_error_str,
                bvs.size(), 1 + index + sizeof...(ArgTs)));
            TI_RAISE(e);
        }
        auto a1 = BoxedValue::boxCast<Arg1>(bvs[index]);

        std::function<Ret(ArgTs...)> _f = [a1, f](ArgTs &&... args) {
            return f(static_cast<Arg1>(*a1), std::forward<ArgTs>(args)...);
        };
        return _bindArguments<index + 1>(_f, bvs);
    }

    template <int index>
    static std::function<Ret()> _bindArguments(std::function<Ret()> &f,
                                               std::vector<BoxedValue> &bvs) {
        if (index != bvs.size()) {
            auto e = thallium::exception::ParameterError(format(arity_error_str,
                bvs.size(), index));
            TI_RAISE(e);
        }
        return f;
    }
};

template <class T>
class FunctionObjectImpl;

template <class Ret, class... ArgTypes>
class FunctionObjectImpl<Ret(ArgTypes...)> : public FunctionObjectBase {
    using FuncSignature = FunctionSingnature<Ret(ArgTypes...)>;
    std::function<Ret(ArgTypes...)> f;

  public:
    FunctionObjectImpl(Ret(func)(ArgTypes...))
        : FunctionObjectBase(), f(func) {}

    FunctionObjectImpl(const std::function<Ret(ArgTypes...)> &func)
        : FunctionObjectBase(), f(func) {}

    std::vector<BoxedValue> deSerializeArguments(const Buffers &bs) override {
        return FuncSignature::deSerializeArguments(bs);
    }

    BoxedValue operator()(std::vector<BoxedValue> &args) override {
        auto f_bd = FuncSignature::bindArguments(f, args);
        return f_bd();
    }
};

template <class Ret, class... ArgTypes>
class VoidFunctionObject : public FunctionObjectImpl<Ret(ArgTypes...)> {
    static_assert(std::is_same<Ret, void>::value,
                  "Void function's return type must be void!");
};

template <class Ret, class... ArgTypes>
using FunctionObject = FunctionObjectImpl<Ret(ArgTypes...)>;

template <class Ret, class... ArgTypes>
FunctionObjectImpl<Ret(ArgTypes...)> make_function_object(
    Ret(func)(ArgTypes...)) {
    return FunctionObjectImpl<Ret(ArgTypes...)>(func);
}
template <class Ret, class... ArgTypes>
FunctionObjectImpl<Ret(ArgTypes...)> make_function_object(
    const std::function<Ret(ArgTypes...)> &func) {
    return FunctionObjectImpl<Ret(ArgTypes...)>(func);
}
template <class... ArgTypes>
FunctionObjectImpl<void(ArgTypes...)> make_void_function_object(
    void(func)(ArgTypes...)) {
    return VoidFunctionObject<void(ArgTypes...)>(func);
}
template <class... ArgTypes>
FunctionObjectImpl<void(ArgTypes...)> make_void_function_object(
    const std::function<void(ArgTypes...)> &func) {
    return VoidFunctionObject<void(ArgTypes...)>(func);
}

class FuncManager : public Singleton<FuncManager> {
    friend class Singleton<FuncManager>;
    std::unordered_map<FuncId, std::shared_ptr<FunctionObjectBase>> f_table;

  public:
    FuncManager() {}

    void addFunc(const FuncId &f_id,
                 const std::shared_ptr<FunctionObjectBase> &f_ptr);

    std::shared_ptr<FunctionObjectBase> getFuncObj(const FuncId &f_id);
};

template <class Ret, class... ArgTypes>
void register_func(Ret(func)(
    ArgTypes...)) {  // redundant codes, refectoring after future changes.
    auto _p = std::make_shared<FunctionObject<Ret, ArgTypes...>>(func);
    auto p = std::dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get().addFunc(f_id, p);
}

template <class Ret, class... ArgTypes>
void register_func(std::function<Ret(ArgTypes...)> &func) {
    auto _p = std::make_shared<FunctionObject<Ret, ArgTypes...>>(func);
    auto p = std::dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get().addFunc(f_id, p);
}

template <class... ArgTypes>
void register_void_func(void(func)(ArgTypes...)) {
    auto _p = std::make_shared<VoidFunctionObject<ArgTypes...>>(func);
    auto p = std::dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get().addFunc(f_id, p);
}

template <class... ArgTypes>
void register_void_func(std::function<void(ArgTypes...)> &func) {
    auto _p = std::make_shared<VoidFunctionObject<ArgTypes...>>(func);
    auto p = std::dynamic_pointer_cast<FunctionObjectBase>(_p);
    auto f_id = function_id(func);
    FuncManager::get().addFunc(f_id, p);
}

std::shared_ptr<FunctionObjectBase> get_function_object(const FuncId &f_id);

_THALLIUM_END_NAMESPACE

#endif
