#ifndef _THALLIUM_SERIALIZE_HPP
#define _THALLIUM_SERIALIZE_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "common.hpp"
_THALLIUM_BEGIN_NAMESPACE

typedef std::string Buffer;
using Buffers = std::vector<std::string>;
using BuffersPtr = std::unique_ptr<Buffers>;

template <typename T>
std::string my_serialized_method(T &a) { // TODO: user implement serialization api
  return to_string(a);
}

std::string my_serialized_method(std::string &a);

namespace Serializer {

template <class T> std::string serialize(T t) {
  return my_serialized_method(t); // TODO
}

template <class... ArgTypes> BuffersPtr serializeList(const ArgTypes&... args) {
  return BuffersPtr{new Buffers{my_serialized_method(args)...}};
}
template <class T> T deSerialize(std::string s) { return static_cast<T>(s); }
template <> int deSerialize<int>(std::string s);
template <> double deSerialize<double>(std::string s);
template <> long double deSerialize<long double>(std::string s);
template <> float deSerialize<float>(std::string s); 
template <> char deSerialize<char>(std::string s);
} // namespace Serializer

_THALLIUM_END_NAMESPACE

#endif
