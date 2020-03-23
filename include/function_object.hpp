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
} // namespace exception

class FunctionObject {
protected:
  virtual void doCall() = 0;

public:
  template <class Ret, class... ArgTypes> Ret operator()(ArgTypes... args) {}
};

template <class Ret, class... ArgTypes>
class FunctionObjectImpl : public FunctionObject {};

template <class Ret, class... ArgTypes> class FunctionSingnature {
public:
  static vector<BoxValue> deSerializeArguments(const vector<string> &buffers) {
    auto bvs = vector<BoxValue>{};
    _deSerializeArguments<0, ArgTypes...>(bvs, buffers);
    return bvs;
  }

private:
  template <int index, class Arg1, class... ArgTs>
  static void _deSerializeArguments(vector<BoxValue> &bvs,
                                    const vector<string> &buffers) {
    if (index + 1 > buffers.size()) {
      throw thallium::exception::ParameterError(
          "Function arguments arity doesnt match."); // TODO: traceback and <<
                                                     // operator
    }
    bvs.push_back(Serializer::deSerialize<Arg1>(buffers[index]));
    _deSerializeArguments<index + 1, ArgTs...>(bvs, buffers);
  }
  template <int index>
  static void _deSerializeArguments(vector<BoxValue> &bvs,
                                    const vector<string> &buffers) {
    if (bvs.size() != buffers.size()) {
      throw thallium::exception::ParameterError(
          "Function arguments arity doesnt match...........");
    }
  }
};

_THALLIUM_END_NAMESPACE

#include <typeinfo>

int main() {

  auto s = thallium::FunctionSingnature<int, int, float, double>{};
  auto bvs =
      s.deSerializeArguments(std::vector<std::string>{"2", "3.14", "5.453"});
  std::cout << bvs.size() << std::endl;
  std::cout << boost::any_cast<int>(bvs[0]) << std::endl;
  std::cout << boost::any_cast<float>(bvs[1]) << std::endl;
  std::cout << boost::any_cast<double>(bvs[2]) << std::endl;
  return 0;
}
