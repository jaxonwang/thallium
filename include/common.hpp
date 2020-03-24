
#ifndef _THALLIUM_COMMON
#define _THALLIUM_COMMON

#define _THALLIUM_BEGIN_NAMESPACE namespace thallium {
#define _THALLIUM_END_NAMESPACE }

#include <exception>
#include <sstream>
#include <string>
#include <typeinfo>

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

template <class Rt, class... Argtypes>
const string function_id(Rt(f)(Argtypes...)) {
    return typeid(f).name();
}

namespace ti_exception {
class format_error : public runtime_error {
   public:
    explicit format_error(const string &what_arg) : runtime_error(what_arg) {}
};
}  // namespace ti_exception

inline void __raise_left_brace_unmatch(const string &fmt) {
    throw ti_exception::format_error("Expecting \'{\' in the brace pair: " +
                                     fmt);
}
inline void __raise_right_brace_unmatch(const string &fmt) {
    throw ti_exception::format_error("Expecting \'}\' in the brace pair: " +
                                     fmt);
}

template <class T>
inline void __raise_too_many_args(T &t) {
    stringstream tss{"Too many argumetns given: "};
    tss << t;
    throw ti_exception::format_error(tss.str());
}

inline void _format(stringstream &ss, const string &fmt, unsigned long pos) {
    for (; pos < fmt.size(); pos++) {
        bool islast = (pos + 1 == fmt.size());
        char current = fmt[pos];
        if (islast) {
            if (fmt[pos] == '{') {
                __raise_right_brace_unmatch(fmt);
            } else if (fmt[pos] == '}') {
                __raise_left_brace_unmatch(fmt);
            }
        } else if (fmt[pos] == '{') {
            if (fmt[pos + 1] == '{') {
                pos++;
            } else if (fmt[pos + 1] == '}') {
                throw ti_exception::format_error(
                    "Too many format positions '{}'" + fmt);
            } else {
                __raise_right_brace_unmatch(fmt);
            }
        } else if (fmt[pos] == '}') {
            if (fmt[pos + 1] == '}') {
                pos++;
            } else {
                __raise_left_brace_unmatch(fmt);
            }
        }
        ss << current;
    }
}

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
string format(const string &fmt, const Args &... args) {
    stringstream ss;
    _format(ss, fmt, 0, args...);
    return ss.str();
}

_THALLIUM_END_NAMESPACE

#endif
