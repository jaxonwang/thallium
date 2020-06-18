
#ifndef _THALLIUM_COMMON
#define _THALLIUM_COMMON

#define _THALLIUM_BEGIN_NAMESPACE namespace thallium {
#define _THALLIUM_END_NAMESPACE }

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <cstring>

#include "exception.hpp"

using namespace std;

_THALLIUM_BEGIN_NAMESPACE

namespace ti_exception {
class format_error : public runtime_error {
   public:
    explicit format_error(const string &what_arg) : runtime_error(what_arg) {}
};
}  // namespace ti_exception

inline void __raise_left_brace_unmatch(const string &fmt) {
    TI_RAISE(ti_exception::format_error("Expecting \'{\' in the brace pair: " +
                                     fmt));
}
inline void __raise_right_brace_unmatch(const string &fmt) {
    TI_RAISE(ti_exception::format_error("Expecting \'}\' in the brace pair: " +
                                     fmt));
}

template <class T>
inline void __raise_too_many_args(T &t) {
    stringstream tss{"Too many argumetns given: "};
    tss << t;
    TI_RAISE(ti_exception::format_error(tss.str()));
}

void _format(stringstream &ss, const string &fmt, unsigned long pos);

template <class T,
          class = decltype(declval<ostream &>()
                           << declval<T>()),  // ensure << op exist
          class... Args>
inline void _format(stringstream &ss, const string &fmt, unsigned long pos,
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
inline string format(const string &fmt, const Args &... args) {
    stringstream ss;
    _format(ss, fmt, 0, args...);
    return ss.str();
}

template <class>
struct is_string: std::false_type{};

template <class CharT>
struct is_string<basic_string<CharT>>: std::true_type{};


template <class String>
void string_trim_modified(String &s, const String &cutset=""){
    static_assert(is_string<String>::value, "Parameter type must be string");
    String _cutset = cutset;

    if(_cutset.emtpy()) 
        _cutset = " \t\n\r";

    auto l_pos = s.find_first_not_of(_cutset);
    auto r_pos = s.find_last_not_of(_cutset);
    auto count = r_pos - l_pos + 1;
    s.erase(l_pos, count);
}

template <class String>
String string_trim(const String &s, const String &cutset=""){
    static_assert(is_string<String>::value, "Parameter type must be string");
    String _s = s;
    string_trim_modified(_s, cutset);
    return _s; 
}

template <class String, class StrList> // StrList must be SequenceContainer
String string_join(const StrList &string_list, const String &sep){
    static_assert(is_same<typename StrList::value_type, String>::value, "The seperator and strings in string list must be the same type!");
    static_assert(is_string<String>::value, "Parameter type must be string");

    using SizeType = typename String::size_type;
    using CharT= typename String::value_type;

    SizeType total_length = 0;
    SizeType sep_size = sep.size();

    if(string_list.size() == 0) {// the vectors are empty
        return String{};
    }

    for(auto &s: string_list){
        total_length += s.size();
    }

    total_length += (string_list.size() - 1) * sep_size;
    CharT * new_str = new CharT[total_length + 1]{0};
    CharT * pos = new_str;
    const CharT * sep_ptr = sep.c_str();

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
    delete [] new_str;

    return ret;
}

template <class CharT, class StrList, class String = typename StrList::value_type> // accept when sep is cstyle string 
String string_join(const StrList &string_list, const CharT * sep){
    static_assert(is_same<typename String::value_type, CharT>::value, "The seperator char type and the char type in string list must be the same!");
    String ret;
    if(!sep){
        // if sep null, should not feed to constructor to avoid UB
        ret = string_join(string_list, String{});
    }else{
        ret = string_join(string_list, String{sep});
    }
    return ret;
}


_THALLIUM_END_NAMESPACE

#endif
