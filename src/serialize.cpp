#include "serialize.hpp"

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
string my_serialized_method(string &a) { return a; }

namespace Serializer {
template <>
int deSerialize<int>(string s) {
    return stoi(s);
}
template <>
double deSerialize<double>(string s) {
    return stod(s);
}
template <>
long double deSerialize<long double>(string s) {
    return stold(s);
}
template <>
float deSerialize<float>(string s) {
    return stof(s);
}
template <>
char deSerialize<char>(string s) {
    return s[0];
}

StringSaveArchive::StringSaveArchive()
    : capacity(64),
      _buf(new char[capacity]),
      _buf_begin(_buf.get()),
      save_cursor(_buf_begin) {}

char *StringSaveArchive::get_save_cursor(const size_t num) {
    size_t _size_used = save_cursor - _buf_begin;
    if (_size_used + num > capacity) {
        size_t new_capacity = std::max(_size_used + num, capacity * 2);
        unique_ptr<char[]> new_buf{new char[new_capacity]};
        char *src = _buf.get();
        std::copy(src, src + _size_used, new_buf.get());
        capacity = new_capacity;
        _buf = move(new_buf);
        _buf_begin = _buf.get();
        save_cursor = _buf_begin + _size_used;
    }
    char *ret = save_cursor;
    save_cursor += num;
    return ret;
}

string StringSaveArchive::build() { return string(_buf_begin, save_cursor); }

StringLoadArchive::StringLoadArchive(const string &s)
    : _buf_begin(s.c_str()), load_cursor(_buf_begin), _buf_size(s.size()) {}

const char *StringLoadArchive::get_load_cursor(const size_t num) {
    const char *ret = load_cursor;
    const size_t result = load_cursor - _buf_begin + num;
    if (result > _buf_size) {
        throw logic_error("buf load overflow!");
    }
    load_cursor += num;
    return ret;
}

}  // namespace Serializer

_THALLIUM_END_NAMESPACE
