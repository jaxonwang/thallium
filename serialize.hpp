#ifndef _THALLIUM_SERIALIZE_HPP
#define _THALLIUM_SERIALIZE_HPP

#include <memory>
#include <vector>

#include "common.hpp"
_THALLIUM_BEGIN_NAMESPACE

using namespace std;

using Buffers = vector<string>;
using BuffersPtr = unique_ptr<Buffers>;

template <typename T>
string my_serialized_method(T &a) { // TODO: user implement serialization api
  return to_string(a);
}

string my_serialized_method(string &a) { return a; }

namespace Serializer {

template <class T> string serialize(T t) {
  return my_serialized_method(t); // TODO
}

template <class... ArgTypes> BuffersPtr serializeList(ArgTypes... args) {
  return BuffersPtr{new Buffers{my_serialized_method(args)...}};
}
template <class T> T deSerialize(string s) { return static_cast<T>(s); }
template <> int deSerialize<int>(string s) { return stoi(s); }
template <> double deSerialize<double>(string s) {
  std::cout << s << std::endl;
  double k = static_cast<double>(std::stof(s));
  std::cout << static_cast<double>(std::stof("5.453"))<<std::endl;
  std::cout << static_cast<double>(std::stof(s))<<std::endl;
  std::cout << k << std::endl;
  return k;
}
template <> float deSerialize<float>(string s) { return stof(s); }
} // namespace Serializer

_THALLIUM_END_NAMESPACE

#endif
