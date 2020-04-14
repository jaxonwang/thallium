#ifndef _THALLIUM_SERIALIZE_HPP
#define _THALLIUM_SERIALIZE_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "common.hpp"
_THALLIUM_BEGIN_NAMESPACE

typedef string Buffer;
using Buffers = vector<string>;
using BuffersPtr = unique_ptr<Buffers>;

template <typename T>
string my_serialized_method(T &a) { // TODO: user implement serialization api
  return to_string(a);
}

string my_serialized_method(string &a);

namespace Serializer {

template <class T> string serialize(T t) {
  return my_serialized_method(t); // TODO
}

template <class... ArgTypes> BuffersPtr serializeList(const ArgTypes&... args) {
  return BuffersPtr{new Buffers{my_serialized_method(args)...}};
}
template <class T> T deSerialize(string s) { return static_cast<T>(s); }
template <> int deSerialize<int>(string s);
template <> double deSerialize<double>(string s);
template <> long double deSerialize<long double>(string s);
template <> float deSerialize<float>(string s); 
template <> char deSerialize<char>(string s);
} // namespace Serializer

_THALLIUM_END_NAMESPACE

#endif
