#ifndef _THALLIUM_SERIALIZE_HPP
#define _THALLIUM_SERIALIZE_HPP

#include <iostream>
#include <list>
#include <memory>
#include <type_traits>
#include <vector>
#include <unordered_map>

#include "common.hpp"
#include "gsl/span.hpp"
_THALLIUM_BEGIN_NAMESPACE

typedef std::string Buffer;
using Buffers = std::vector<std::string>;
using BuffersPtr = std::unique_ptr<Buffers>;

namespace Serializer {  // TODO rename to serializer

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
struct is_savearchive {
    constexpr static bool value =
        std::is_base_of<SaveArchive<Archive>, Archive>::value;
};

template <class Archive>
struct is_loadarchive {
    constexpr static bool value =
        std::is_base_of<LoadArchive<Archive>, Archive>::value;
};

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
        a << element;  // use << instead of call serialize because proper
                       // serialize might not visible here
    }
}

template <class Archive>
using enable_if_savearchive_t =
    typename std::enable_if<is_savearchive<Archive>::value, int>::type;

template <class Archive>
using enable_if_loadarchive_t =
    typename std::enable_if<is_loadarchive<Archive>::value, int>::type;

template <
    class Archive, class T, enable_if_savearchive_t<Archive> = 0,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void serialize(Archive &a, const T t) {
    char *dst = a.get_save_cursor(sizeof(T));
    const char *src = reinterpret_cast<const char *>(&t);
    std::copy(src, src + sizeof(T), dst);
}

template <class Archive, class T, size_t N,
          enable_if_savearchive_t<Archive> = 0>
void serialize(Archive &a, const gsl::static_span<T, N> &sp) {
    _span_serialize(a, sp);
}

template <class Archive, class T, enable_if_savearchive_t<Archive> = 0>
void serialize(Archive &a, const gsl::span<T> &sp) {
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

template <class Archive, class T, size_t N,
          enable_if_loadarchive_t<Archive> = 0>
void deserialize(Archive &a, gsl::static_span<T, N> &sp) {
    _span_deserialize(a, sp);
}

// vector, string, go here
template <class Archive,
          class Resizeable,  // Resizeable should be linear storage
          enable_if_loadarchive_t<Archive> = 0,
          typename std::enable_if<!is_trivially_serializable<Resizeable>::value,
                                  int>::type = 0>
void deserialize(Archive &a, Resizeable &r) {
    size_t size_to_increase;
    a >> size_to_increase;
    r.resize(size_to_increase);
    if (size_to_increase > 0) {
        auto sp = gsl::make_span(&r[0], size_to_increase);
        _span_deserialize(a, sp);
    }
}

template <
    class Archive, class T, enable_if_loadarchive_t<Archive> = 0,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
void deserialize(Archive &a, T &t) {
    const char *src = a.get_load_cursor(sizeof(T));
    char *dst = reinterpret_cast<char *>(&t);
    std::copy(src, src + sizeof(T), dst);
}

template <class T>
struct has_serializable_member {
    struct fake_archive {
        template<class K>
        fake_archive &operator&(K &) { return *this; }
    };
    template <class U>
    using _function_t = decltype(&U::template serializable<fake_archive>);
    template <class U>
    using is_correct_t = typename std::enable_if<
        std::is_same<_function_t<U>, void (U::*)(fake_archive &)>::value,
        int>::type;
    template <class U>
    static char test(is_correct_t<U>);
    template <class>
    static u_int64_t test(...);
    constexpr static bool value = sizeof(test<T>(0)) == 1;
};

template <
    class Archive, class T, enable_if_savearchive_t<Archive> = 0,
    typename std::enable_if<!has_serializable_member<T>::value, int>::type = 0>
Archive &operator<<(Archive &a, const T &t) {
    serialize(a, t);
    return a;
}

template <
    class Archive, class T, enable_if_savearchive_t<Archive> = 0,
    typename std::enable_if<has_serializable_member<T>::value, int>::type = 0>
Archive &operator<<(Archive &a, const T &t) {
    const_cast<T &>(t).serializable(a);  // danger!
    return a;
}

template <class Archive, class T, size_t N,
          enable_if_savearchive_t<Archive> = 0>
Archive &operator<<(Archive &a, const T (&t)[N]) {
    auto sp = gsl::make_static_span(t);
    serialize(a, sp);
    return a;
}

template <class Archive, enable_if_savearchive_t<Archive> = 0>
Archive &operator<<(Archive &a, const std::string &s) {
    auto sp = gsl::make_span(s);
    serialize(a, sp);
    return a;
}

template <class Archive, class T, enable_if_savearchive_t<Archive> = 0>
Archive &operator<<(Archive &a, const std::vector<T> &v) {
    gsl::span<const T> sp{v.data(), v.size()};
    serialize(a, sp);
    return a;
}

template <class Archive, class K, class V, enable_if_savearchive_t<Archive> = 0>
Archive &operator<<(Archive &a, const std::unordered_map<K, V> &map) {
    size_t length = map.size();
    a << length;
    for (auto &kv : map) {
        a << kv.first;
        a << kv.second;
    }
    return a;
}

template <class Archive, enable_if_loadarchive_t<Archive> = 0>
Archive &operator>>(Archive &a, std::string &s) {
    deserialize(a, s);
    return a;
}

template <class Archive, class T, enable_if_loadarchive_t<Archive> = 0>
Archive &operator>>(Archive &a, std::vector<T> &v) {
    deserialize(a, v);
    return a;
}

template <
    class Archive, class T, enable_if_loadarchive_t<Archive> = 0,
    typename std::enable_if<!has_serializable_member<T>::value, int>::type = 0>
Archive &operator>>(Archive &a, T &t) {
    deserialize(a, t);
    return a;
}

template <
    class Archive, class T, enable_if_loadarchive_t<Archive> = 0,
    typename std::enable_if<has_serializable_member<T>::value, int>::type = 0>
Archive &operator>>(Archive &a, T &t) {
    t.serializable(a);
    return a;
}

template <class Archive, class T, size_t N,
          enable_if_loadarchive_t<Archive> = 0>
Archive &operator>>(Archive &a, T (&t)[N]) {
    auto sp = gsl::make_static_span(t);
    deserialize(a, sp);
    return a;
}

template <class Archive, class K, class V, enable_if_loadarchive_t<Archive> = 0>
Archive &operator>>(Archive &a, std::unordered_map<K, V> &map) {
    size_t length;
    a >> length;
    for (size_t i = 0; i < length; i++) {
        K k;
        V v;
        a >> k;
        a >> v;
        map[k] = v;
    }
    return a;
}

//  below is declarations

template <class T, typename std::enable_if<is_trivially_serializable<T>::value,
                                           int>::type = 0>
constexpr size_t real_size(const T &);

template <
    class T,
    typename std::enable_if<!is_trivially_serializable<T>::value, int>::type=0,
    typename std::enable_if<has_serializable_member<T>::value, int>::type=0>
size_t real_size(const T &t);

template <
    class T, size_t N,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type = 0>
constexpr size_t real_size(const T (&)[N]);

template <class T, size_t N,
          typename std::enable_if<!is_trivially_serializable<T>::value,
                                  int>::type = 0>
constexpr size_t real_size(const T (&arr)[N]);

template <class T, typename std::enable_if<!is_trivially_serializable<T>::value,
                                           int>::type = 0>
size_t real_size(const gsl::span<T> &sp);

template <class T, typename std::enable_if<is_trivially_serializable<T>::value,
                                           int>::type = 0>
size_t real_size(const gsl::span<T> &sp);

size_t real_size(const std::string &s);

template <class T>
size_t real_size(const std::vector<T> &v);

template<class K, class V>
size_t real_size(const std::unordered_map<K,V> &m);

class SizeArchive {
  private:
    size_t total_size;

  public:
    constexpr SizeArchive() : total_size(0) {}
    template <class T>
    SizeArchive &operator&(const T &t) {
        total_size += real_size(t);
        return *this;
    }
    size_t get_total_size() const { return total_size; }
};

template <class T, typename std::enable_if<is_trivially_serializable<T>::value,
                                           int>::type>
constexpr size_t real_size(const T &) {
    return sizeof(T);
}

template <
    class T, size_t N,
    typename std::enable_if<is_trivially_serializable<T>::value, int>::type>
constexpr size_t real_size(const T (&)[N]) {
    return N * sizeof(T);
}

template <
    class T,
    typename std::enable_if<!is_trivially_serializable<T>::value, int>::type,
    typename std::enable_if<has_serializable_member<T>::value, int>::type>
size_t real_size(const T &t) {
    SizeArchive sz_a;
    const_cast<T &>(t).serializable(sz_a);
    return sz_a.get_total_size();  // danger but have to
}

template <class T, typename std::enable_if<!is_trivially_serializable<T>::value,
                                           int>::type>
size_t real_size(const gsl::span<T> &sp) {
    // no optimization for static_span, since we have to recurisve call the sum,
    // very slow compiling
    size_t sum = sizeof(size_t);
    for (auto &i : sp) {
        sum += real_size(i);
    }
    return sum;
}

template <class T, typename std::enable_if<is_trivially_serializable<T>::value,
                                           int>::type>
size_t real_size(const gsl::span<T> &sp) {
    return sizeof(T) * sp.size() + sizeof(size_t);
}

template <
    class T, size_t N,
    typename std::enable_if<!is_trivially_serializable<T>::value, int>::type>
constexpr size_t real_size(const T (&arr)[N]) {
    // minus a size_t since real_size span will account length for dynamic span
    return real_size(gsl::make_span(arr)) - sizeof(size_t);
}

template <class T>
size_t real_size(const std::vector<T> &v) {
    return real_size(gsl::make_span(v));
}

template<class K, class V>
size_t real_size(const std::unordered_map<K,V> &m){
    size_t sum = sizeof(size_t);
    for (auto &kv : m) {
        sum += real_size(kv.first);
        sum += real_size(kv.second);
    }    
    return sum;
}

// below is used by boxedvalue and submitter
template <class T>
T create_from_string(const std::string &s) {
    StringLoadArchive la{s};
    T t;
    la >> t;
    return t;
}

template <class T>
std::string create_string(const T &t) {
    StringSaveArchive sa;
    sa << t;
    return sa.build();
}

template <class... ArgTypes>
BuffersPtr serializeList(const ArgTypes &... args) {
    return BuffersPtr{new Buffers{create_string(args)...}};
}

}  // namespace Serializer

_THALLIUM_END_NAMESPACE

#endif
