#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

namespace message {

void header_to_string(const Header &h, char *buf) { *(Header *)buf = h; }

Header string_to_header(const char *buf) {
    Header h = *(Header *)buf;
    return h;
}

ZeroCopyBuffer::ZeroCopyBuffer(typename CopyableBuffer::size_type size,
                               typename CopyableBuffer::value_type value)
    : CopyableBuffer(size, value) {}

ZeroCopyBuffer::ZeroCopyBuffer(ZeroCopyBuffer &&other)
    : CopyableBuffer(move(other)) {}

CopyableBuffer &ZeroCopyBuffer::to_copyable() {
    return static_cast<CopyableBuffer &>(*this);
}

}  // namespace message

_THALLIUM_END_NAMESPACE
