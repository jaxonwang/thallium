#include "common.hpp"

using namespace std;

_THALLIUM_BEGIN_NAMESPACE

void _format(stringstream &ss, const string &fmt, unsigned long pos) {
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
                TI_RAISE(ti_exception::format_error(
                    "Too many format positions '{}'" + fmt));
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

_THALLIUM_END_NAMESPACE
