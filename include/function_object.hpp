#include <boost/any.hpp>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>

#include "common.hpp"
#include "serialize.hpp"

_THALLIUM_BEGIN_NAMESPACE

using BoxValue = boost::any;

namespace exception {
class ParameterError : public std::range_error {
   public:
    ParameterError(const std::string &what) : range_error(what) {}
};
}  // namespace exception

class FunctionObject {
   protected:
    virtual void doCall() = 0;

   public:
    template <class Ret, class... ArgTypes>
    Ret operator()(ArgTypes... args) {}
};

template <class Ret, class... ArgTypes>
class FunctionObjectImpl : public FunctionObject {};

template <class Ret, class... ArgTypes>
class FunctionSingnature {
   public:
    static vector<BoxValue> deSerializeArguments(
        const vector<string> &buffers) {
        auto bvs = vector<BoxValue>{};
        _deSerializeArguments<0, ArgTypes...>(bvs, buffers);
        return bvs;
    }

   private:
    template <int index, class Arg1, class... ArgTs>
    static void _deSerializeArguments(vector<BoxValue> &bvs,
                                      const vector<string> &buffers) {
        if (index + 1 > buffers.size()) {
            throw thallium::exception::ParameterError(format(
                "Function arguments arity doesnt match: given {}, need {}",
                buffers.size(), 1 + index + sizeof...(ArgTs)));
        }
        bvs.push_back(Serializer::deSerialize<Arg1>(buffers[index]));
        _deSerializeArguments<index + 1, ArgTs...>(bvs, buffers);
    }
    template <int index>
    static void _deSerializeArguments(vector<BoxValue> &bvs,
                                      const vector<string> &buffers) {
        if (bvs.size() != buffers.size()) {
            throw thallium::exception::ParameterError(format(
                "Function arguments arity doesnt match: given {}, need {}",
                buffers.size(), index));
        }
    }
};

_THALLIUM_END_NAMESPACE
