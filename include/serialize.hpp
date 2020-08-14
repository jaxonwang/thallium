#ifndef _THALLIUM_SERIALIZE_HPP
#define _THALLIUM_SERIALIZE_HPP

#include <iostream>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>

#include "common.hpp"
#include "gsl/span.hpp"
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

template <class ConcreteArchive>
class SaveArchive {
  public:
    template <class T>
    ConcreteArchive &operator&(const T &t) {
        return *static_cast<ConcreteArchive *>(this) << t;
    }
};

template <class ConcreteArchive>
class LoadArchive {
  public:
    template <class T>
    ConcreteArchive &operator&(T &t) {
        return *static_cast<ConcreteArchive *>(this) >> t;
    }
};

template <class Archive>
inline void assert_is_savearchive() {
    static_assert(std::is_base_of<SaveArchive<Archive>, Archive>::value,
                  "Must be save archive!");
}

template <class Archive>
inline void assert_is_loadarchive() {
    static_assert(std::is_base_of<LoadArchive<Archive>, Archive>::value,
                  "Must be load archive!");
}

class StringSaveArchive : public SaveArchive<StringSaveArchive> {
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

class StringLoadArchive : public LoadArchive<StringLoadArchive> {
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
    assert_is_savearchive<Archive>();
    char *dst = a.get_save_cursor(sizeof(T));
    const char *src = reinterpret_cast<const char *>(&t);
    std::copy(src, src + sizeof(T), dst);
}

template <
    class Archive, class T,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void _span_serialize(Archive &a, const gsl::span<T> &sp) {
    char *dst = a.get_save_cursor(sp.size_bytes());
    const char *src = reinterpret_cast<const char *>(sp.data());
    std::copy(src, src + sp.size_bytes(), dst);
}

template <class Archive, class T,
          typename std::enable_if<!is_trivially_serializable<T>::value,
                                  int>::type = 0>
void _span_serialize(Archive &a, const gsl::span<T> &sp) {
    for (auto &element : sp) {
        a << element; // use << instead of call serialize because proper serialize might not visible here
    }
}

template <class Archive, class T, size_t N>
void serialize(Archive &a, const gsl::static_span<T, N> &sp) {
    assert_is_savearchive<Archive>();
    _span_serialize(a, sp);
}

template <class Archive, class T>
void serialize(Archive &a, const gsl::span<T> &sp) {
    assert_is_savearchive<Archive>();
    a << sp.size();
    _span_serialize(a, sp);
}

template <
    class Archive, class T,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void _span_deserialize(Archive &a, gsl::span<T> &sp) {
    const char *src = a.get_load_cursor(sp.size_bytes());
    char *dst = reinterpret_cast<char *>(sp.data());
    std::copy(src, src + sp.size_bytes(), dst);
}

template <class Archive, class T,
          typename std::enable_if<!is_trivially_serializable<T>::value,
                                  int>::type = 0>
void _span_deserialize(Archive &a, gsl::span<T> &sp) {
    for (auto &element : sp) {
        a >> element;
    }
}

template <class Archive, class T, size_t N>
void deserialize(Archive &a, gsl::static_span<T, N> &sp) {
    assert_is_loadarchive<Archive>();
    _span_deserialize(a, sp);
}

template <class Archive,
          class Resizeable,  // Resizeable should be linear storage
          typename std::enable_if<!is_trivially_serializable<Resizeable>::value,
                                  int>::type = 0>
void deserialize(Archive &a, Resizeable &r) {
    assert_is_loadarchive<Archive>();
    size_t size_to_increase;
    a >> size_to_increase;
    r.resize(size_to_increase);
    if (size_to_increase > 0) {
        auto sp = gsl::make_span(&r[0], size_to_increase);
        _span_deserialize(a, sp);
    }
}

template <
    class Archive, class T,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void deserialize(Archive &a, T &t) {
    assert_is_loadarchive<Archive>();
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
Archive &operator<<(Archive &a, const T &t) {
    assert_is_savearchive<Archive>();
    serialize(a, t);
    return a;
}

template <
    class Archive, class T,
    typename std::enable_if<has_serializable_member<T>::value, int>::type = 0>
Archive &operator<<(Archive &a, const T &t) {
    assert_is_savearchive<Archive>();
    const_cast<T &>(t).serializable(a);  // danger!
    return a;
}

template <class Archive, class T, size_t N>
Archive &operator<<(Archive &a, const T (&t)[N]) {
    assert_is_savearchive<Archive>();
    auto sp = gsl::make_static_span(t);
    serialize(a, sp);
    return a;
}

template <class Archive>
Archive &operator<<(Archive &a, const std::string &s) {
    assert_is_savearchive<Archive>();
    auto sp = gsl::make_span(s);
    serialize(a, sp);
    return a;
}

template <class Archive>
Archive &operator>>(Archive &a, std::string &s) {
    assert_is_loadarchive<Archive>();
    deserialize(a, s);
    return a;
}

template <class Archive, class T>
Archive &operator<<(Archive &a, const std::vector<T> &v) {
    assert_is_savearchive<Archive>();
    gsl::span<const T> sp{v.data(), v.size()};
    serialize(a, sp);
    return a;
}

template <class Archive, class T>
Archive &operator>>(Archive &a, std::vector<T> &v) {
    assert_is_loadarchive<Archive>();
    deserialize(a, v);
    return a;
}

template <
    class Archive, class T,
    typename std::enable_if<!has_serializable_member<T>::value, int>::type = 0>
Archive &operator>>(Archive &a, T &t) {
    assert_is_loadarchive<Archive>();
    deserialize(a, t);
    return a;
}

template <
    class Archive, class T,
    typename std::enable_if<has_serializable_member<T>::value, int>::type = 0>
Archive &operator>>(Archive &a, T &t) {
    assert_is_loadarchive<Archive>();
    t.serializable(a);
    return a;
}

template <class Archive, class T, size_t N>
Archive &operator>>(Archive &a, T (&t)[N]) {
    assert_is_loadarchive<Archive>();
    auto sp = gsl::make_static_span(t);
    deserialize(a, sp);
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
