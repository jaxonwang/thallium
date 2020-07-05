
#ifndef _THALLIUM_COMMON
#define _THALLIUM_COMMON

#define _THALLIUM_BEGIN_NAMESPACE namespace thallium {
#define _THALLIUM_END_NAMESPACE }

#include <cstring>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "exception.hpp"

_THALLIUM_BEGIN_NAMESPACE

namespace ti_exception {
class format_error : public std::runtime_error {
  public:
    explicit format_error(const std::string &what_arg) : std::runtime_error(what_arg) {}
};
}  // namespace ti_exception

inline void __raise_left_brace_unmatch(const std::string &fmt) {
    TI_RAISE(ti_exception::format_error("Expecting \'{\' in the brace pair: " +
                                        fmt));
}
inline void __raise_right_brace_unmatch(const std::string &fmt) {
    TI_RAISE(ti_exception::format_error("Expecting \'}\' in the brace pair: " +
                                        fmt));
}

template <class T>
inline void __raise_too_many_args(T &t) {
    std::stringstream tss{"Too many argumetns given: "};
    tss << t;
    TI_RAISE(ti_exception::format_error(tss.str()));
}

void _format(std::stringstream &ss, const std::string &fmt, unsigned long pos);

template <class T,
          class = decltype(std::declval<std::ostream &>()
                           << std::declval<T>()),  // ensure << op exist
          class... Args>
inline void _format(std::stringstream &ss, const std::string &fmt, unsigned long pos,
                    const T t, const Args... args) {
    auto size = fmt.size();
    for (; pos < size; pos++) {
        bool islast = (pos + 1 == size);
        switch (fmt[pos]) {
            case '{':
                if (!islast) {
                    if (fmt[pos + 1] == '{') {
                        ss << fmt[pos];
                        pos++;
                    } else if (fmt[pos + 1] == '}') {
                        ss << t;
                        pos += 2;
                        goto outloop;
                    }
                } else {
                    __raise_right_brace_unmatch(fmt);
                }
                break;
            case '}':
                if (!islast && fmt[pos + 1] == '}') {
                    ss << fmt[pos];
                    pos++;
                } else {
                    __raise_left_brace_unmatch(fmt);
                }
                break;
            default:
                ss << fmt[pos];
                continue;
        };
    }
    __raise_too_many_args(t);

outloop:
    _format(ss, fmt, pos, args...);
}

template <class... Args>
inline std::string format(const std::string &fmt, const Args &... args) {
    std::stringstream ss;
    _format(ss, fmt, 0, args...);
    return ss.str();
}

template <class>
struct __is_string : std::false_type {};

template <class CharT, class Trait, class Allocator>
struct __is_string<std::basic_string<CharT, Trait, Allocator>> : std::true_type {};

template <class T>
struct is_string : __is_string<typename std::remove_cv<T>::type> {};

template <class String,
          typename std::enable_if<!std::is_same<String, String>::value, int>::type = 0>
String *get_default_cutset() {
    static_assert(!std::is_same<String, String>::value,
                  "No default cutset provided. Please specify cutset for trim "
                  "explicitly.");
}

template <class String,
          typename std::enable_if<std::is_same<typename String::value_type, char>::value,
                             int>::type = 0>
const String *get_default_cutset() {
    static String _s;
    if (_s.empty()) _s = String(" \r\t\n\t");
    return &_s;
}

template <
    class String,
    typename std::enable_if<std::is_same<typename String::value_type, wchar_t>::value,
                       int>::type = 0>
const String *get_default_cutset() {
    static String _s;
    if (_s.empty()) _s = String(L" \r\t\n\t");
    return &_s;
}

template <class String>
void string_trim_modified(String &s, const String &cutset) {
    static_assert(is_string<String>::value, "Parameter type must be string");
    String _cutset = cutset;

    if (s.empty()) return;
    if (_cutset.empty()) return;

    auto l_pos = s.find_first_not_of(_cutset);
    decltype(l_pos) count = 0;
    if (l_pos == String::npos) {
        count = 0;
        s.erase(0, s.size());
    } else {
        auto r_pos = s.find_last_not_of(_cutset) + 1;
        if (r_pos < s.size()) {
            count = s.size() - r_pos;
            s.erase(r_pos, count);
        }
        if (l_pos > 0) {
            count = l_pos;
            s.erase(0, count);
        }
    }
}

template <class String>
void string_trim_modified(String &s) {
    string_trim_modified(s, *get_default_cutset<String>());
}

// T type is either pointer to CharT or basic_string<CharT>
template <class T>
struct __infer_string_type {
    static_assert(!std::is_same<T, T>::value, "disable general template");
};

template <class T>
struct __infer_string_type<T *> {
    using type = std::basic_string<typename std::remove_cv<T>::type>;
};

template <class CharT>
struct __infer_string_type<std::basic_string<CharT>> {
    using type = std::basic_string<CharT>;
};

template <class T>
struct infer_string_type
    : __infer_string_type<
          // remove_cv<const char *> == const char *
          typename std::decay<typename std::remove_cv<T>::type>::type> {};

template <class T, typename std::enable_if<is_string<T>::value, int>::type = 0>
T unify(T &s) {
    return s;
}

template <class T, typename std::enable_if<std::is_pointer<T>::value, int>::type = 0,
          class String = typename infer_string_type<T>::type>
String unify(T s) {
    if (!s) return String{};
    return String{s};
}

// unify types of string and cstyle string here
template <class T1, class T2,
          class String = typename infer_string_type<T1>::type>
String string_trim(const T1 &s, const T2 &cutset) {
    String _s = unify(s);
    string_trim_modified(_s, unify(cutset));
    return _s;
}

template <class T1, class String = typename infer_string_type<T1>::type>
String string_trim(const T1 &s) {
    return string_trim(s, *get_default_cutset<String>());
}

template <class String, class StrList>  // StrList must be SequenceContainer
String string_join(const StrList &string_list, const String &sep) {
    static_assert(
        std::is_same<typename StrList::value_type, String>::value,
        "The seperator and strings in string list must be the same type!");
    static_assert(is_string<String>::value, "Parameter type must be string");

    using SizeType = typename String::size_type;
    using CharT = typename String::value_type;

    SizeType total_length = 0;
    SizeType sep_size = sep.size();

    if (string_list.size() == 0) {  // the vectors are empty
        return String{};
    }

    for (auto &s : string_list) {
        total_length += s.size();
    }

    total_length += (string_list.size() - 1) * sep_size;
    CharT *new_str = new CharT[total_length + 1]{0};
    CharT *pos = new_str;
    const CharT *sep_ptr = sep.c_str();

    // i + 1 not consider overflow here
    SizeType i = 0;
    for (; i + 1 < string_list.size(); i++) {
        memcpy(pos, string_list[i].c_str(), string_list[i].size());
        pos += string_list[i].size();
        memcpy(pos, sep_ptr, sep_size);
        pos += sep_size;
    }
    // append_last
    memcpy(pos, string_list[i].c_str(), string_list[i].size());

    String ret{new_str};
    delete[] new_str;

    return ret;
}

template <class CharT, class StrList,
          class String =
              typename StrList::value_type>  // accept when sep is cstyle string
String string_join(const StrList &string_list, const CharT *sep) {
    static_assert(std::is_same<typename String::value_type, CharT>::value,
                  "The seperator char type and the char type in string list "
                  "must be the same!");
    return string_join(string_list, unify(sep));
}

template <template <class, class> class SeqContainer, class String,
          class Allocator = std::allocator<String>>
SeqContainer<String, Allocator> __string_split(const String &s,
                                               const String &substr) {
    SeqContainer<String, Allocator> splited{};
    using size_type = typename String::size_type;
    if (s.size() == 0 || substr.size() == 0) {
        splited.push_back(s);
        return splited;
    }

    size_type pre_pos = 0;
    size_type found = 0;
    while ((found = s.find(substr, pre_pos)) != String::npos) {
        String tmp{s, pre_pos, found - pre_pos};
        splited.push_back(move(tmp));
        pre_pos = found + substr.size();
    }
    splited.push_back(String{s, pre_pos, s.size() - pre_pos});
    return splited;
}

template <
    template <class, class> class SeqContainer, class T1, class T2,
    class Allocator = std::allocator<typename infer_string_type<T1>::type>,
    class String = typename infer_string_type<T1>::type>
SeqContainer<String, Allocator> string_split(const T1 &s, const T2 &substr) {
    return __string_split<SeqContainer>(unify(s), unify(substr));
}

inline bool is_digit(const char c) { return c >= '0' && c <= '9'; }

inline bool is_lowercase(const char c) { return c >= 'a' && c <= 'z'; }

inline bool is_uppercase(const char c) { return c >= 'A' && c <= 'Z'; }

inline bool is_letter(const char c) { return is_lowercase(c) || is_uppercase(c); }

_THALLIUM_END_NAMESPACE

#endif
