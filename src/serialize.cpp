#include "serialize.hpp"
#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
string my_serialized_method(string &a) { return a; }

namespace Serializer {
template <> int deSerialize<int>(string s) { return stoi(s); }
template <> double deSerialize<double>(string s) { return stod(s); }
template <> long double deSerialize<long double>(string s) { return stold(s); }
template <> float deSerialize<float>(string s) { return stof(s); }
template <> char deSerialize<char>(string s) { return s[0]; }
} // namespace Serializer

_THALLIUM_END_NAMESPACE
