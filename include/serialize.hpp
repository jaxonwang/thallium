#ifndef _THALLIUM_SERIALIZE_HPP
#define _THALLIUM_SERIALIZE_HPP

#include <iostream>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>

#include "common.hpp"
_THALLIUM_BEGIN_NAMESPACE

typedef std::string Buffer;
using Buffers = std::vector<std::string>;
using BuffersPtr = std::unique_ptr<Buffers>;

template <typename T>
std::string my_serialized_method(
    T &a) {  // TODO: user implement serialization api
    return to_string(a);
}

std::string my_serialized_method(std::string &a);

namespace Serializer {  // TODO rename to serializer

template <class T>
std::string serialize(T t) {
    return my_serialized_method(t);  // TODO
}

class StringSaveArchive {
  private:
    size_t capacity;
    std::unique_ptr<char[]> _buf;
    char *_buf_begin;
    char *save_cursor;

  public:
    StringSaveArchive();
    size_t size() { return save_cursor - _buf_begin; };
    char *get_save_cursor(const size_t size_to_save);
    std::string build();
};

class StringLoadArchive {
  private:
    const char *_buf_begin;
    const char *load_cursor;
    const size_t _buf_size;

  public:
    StringLoadArchive(const std::string &s);
    size_t size() { return load_cursor - _buf_begin; };
    const char *get_load_cursor(const size_t size_to_load);
};

template <class T>
struct is_trivially_serializable {
    constexpr static bool value =
        std::is_arithmetic<T>::value || std::is_enum<T>::value;
};

template <
    class Archive, class T,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void serialize(Archive &a, const T t) {
    char *dst = a.get_save_cursor(sizeof(T));
    const char *src = reinterpret_cast<const char *>(&t);
    std::copy(src, src + sizeof(T), dst);
}

template <
    class Archive, class T, size_t N,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void serialize(Archive &a, const T (&t)[N]) {
    char *dst = a.get_save_cursor(sizeof(T) * N);
    const char *src = reinterpret_cast<const char *>(&t);
    std::copy(src, src + sizeof(T) * N, dst);
}

template <class Archive, class T, int N,
          typename std::enable_if<!is_trivially_serializable<T>::value,
                                  int>::type = 0>
void serialize(Archive &a, const T (&t)[N]) {
    for (int i = 0; i < N; i++) {
        a << t[i];
    }
}

template <
    class Archive, class T, size_t N,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void deserialize(Archive &a, T (&t)[N]) {
    const char *src = a.get_load_cursor(sizeof(T) * N);
    char *dst = reinterpret_cast<char *>(t);
    std::copy(src, src + sizeof(T) * N, dst);
}

template <class Archive, class T, int N,
          typename std::enable_if<!is_trivially_serializable<T>::value,
                                  int>::type = 0>
void deserialize(Archive &a, T (&t)[N]) {
    for (int i = 0; i < N; i++) {
        a >> t[i];
    }
}

template <
    class Archive, class T,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void deserialize(Archive &a, T &t) {
    const char *src = a.get_load_cursor(sizeof(T));
    char *dst = reinterpret_cast<char *>(&t);
    std::copy(src, src + sizeof(T), dst);
}

template <class T>
struct has_serializable_member {
    struct fake_archive {
        fake_archive &operator&(T &) { return *this; }
    };
    template <class U>
    static char test(decltype(&U::template serializable<fake_archive>));
    template <class>
    static u_int64_t test(...);
    constexpr static bool value = sizeof(test<T>(0)) == 1;
};

template <
    class Archive, class T,
    typename std::enable_if<!has_serializable_member<T>::value, int>::type = 0>
Archive &operator<<(Archive &a, const T t) {
    serialize(a, t);
    return a;
}

template <
    class Archive, class T,
    typename std::enable_if<has_serializable_member<T>::value, int>::type = 0>
Archive &operator<<(Archive &a, const T t) {
    t.serializable(a);
    return a;
}

template <class Archive, class T, size_t N>
Archive &operator<<(Archive &a, const T (&t)[N]) {
    serialize(a, t);
    return a;
}

template <
    class Archive, class T,
    typename std::enable_if<!has_serializable_member<T>::value, int>::type = 0>
Archive &operator>>(Archive &a, T &t) {
    deserialize(a, t);
    return a;
}

template <
    class Archive, class T,
    typename std::enable_if<has_serializable_member<T>::value, int>::type = 0>
Archive &operator>>(Archive &a, T &t) {
    t.serializable(a);
    return a;
}

template <class Archive, class T, size_t N>
Archive &operator>>(Archive &a, T (&t)[N]) {
    deserialize(a, t);
    return a;
}

template <class T,
          typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
size_t real_size(T) {
    return sizeof(T);
}

template <class... ArgTypes>
BuffersPtr serializeList(const ArgTypes &... args) {
    return BuffersPtr{new Buffers{my_serialized_method(args)...}};
}
template <class T>
T deSerialize(std::string s) {  // TODO remove default, use template declaration
    return static_cast<T>(s);
}
template <>
int deSerialize<int>(std::string s);
template <>
double deSerialize<double>(std::string s);
template <>
long double deSerialize<long double>(std::string s);
template <>
float deSerialize<float>(std::string s);
template <>
char deSerialize<char>(std::string s);

}  // namespace Serializer

_THALLIUM_END_NAMESPACE

#endif
