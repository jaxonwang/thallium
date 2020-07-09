#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

namespace message {

void header_to_string(const Header &h, char *buf) { *(Header *)buf = h; }

Header string_to_header(const char *buf) {
    Header h = *(Header *)buf;
    return h;
}

}  // namespace message

_THALLIUM_END_NAMESPACE
