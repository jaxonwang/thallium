#ifndef _THALLIUM_NETWORK_PROTOCOL_HEADER
#define _THALLIUM_NETWORK_PROTOCOL_HEADER

#include <memory>
#include <vector>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

namespace message {

struct Header {         // TODO: change anytime in the future
    u_int8_t msg_type;  // control, async call, ret,other protocol
    uint32_t body_length;
};

enum HeaderMessageType : u_int8_t { heartbeat, normal };

void header_to_string(const Header &h, char *buf);

Header string_to_header(const char *buf);

constexpr size_t header_size = sizeof(Header);

struct Body {
    std::vector<char *> body;
};

typedef std::vector<char> CopyableBuffer;

class ZeroCopyBuffer :public CopyableBuffer{
    public:
    ZeroCopyBuffer(typename CopyableBuffer::size_type, typename CopyableBuffer::value_type);
    ZeroCopyBuffer(const ZeroCopyBuffer &) = delete;
    ZeroCopyBuffer &operator=(const ZeroCopyBuffer &) = delete;
    CopyableBuffer &to_copyable();
};

}  // namespace message

_THALLIUM_END_NAMESPACE
#endif
