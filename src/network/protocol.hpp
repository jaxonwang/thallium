#ifndef _THALLIUM_NETWORK_PROTOCOL_HEADER
#define _THALLIUM_NETWORK_PROTOCOL_HEADER

#include <memory>
#include <type_traits>
#include <vector>

#include "common.hpp"
#include "utils.hpp"

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

class ZeroCopyBuffer : public CopyableBuffer {
    typedef CopyableBuffer::size_type size_type;
    typedef CopyableBuffer::value_type value_type;

  public:
    ZeroCopyBuffer(size_type, value_type = size_type());
    ZeroCopyBuffer(const ZeroCopyBuffer &) = delete;
    ZeroCopyBuffer(CopyableBuffer &&);
    ZeroCopyBuffer(ZeroCopyBuffer &&);
    ZeroCopyBuffer &operator=(const ZeroCopyBuffer &) = delete;
    CopyableBuffer &to_copyable();
};

class ReadOnlyBuffer {
    typedef size_t size_type;
    typedef char value_type;
    const char *const buf;
    size_type buflen;

  public:
    ReadOnlyBuffer() = delete;
    ReadOnlyBuffer(const ReadOnlyBuffer &) = delete;
    ReadOnlyBuffer(ReadOnlyBuffer &&) = delete;
    ReadOnlyBuffer &operator=(const ReadOnlyBuffer &) = delete;
    ReadOnlyBuffer &operator=(ReadOnlyBuffer &&) = delete;

    ReadOnlyBuffer(const value_type *buf, const size_type length)
        : buf(buf), buflen(length) {}
    size_type size() const { return buflen; }
    const value_type *data() const { return buf; }
};

template <class Buffer>
size_t _cast(const Buffer &, const size_t offset) {
    return offset;
}

template <class Buffer, class Arg, class... Args>
size_t _cast(const Buffer &buf, const size_t offset, Arg &arg, Args &... args) {
    static_assert(std::is_standard_layout<tl_remove_cvref<Arg>>::value,
                  "Type should be standerd layout!");
    size_t rest = buf.size() - offset;
    if (rest < sizeof(arg))
        throw std::runtime_error(
            format("bad cast: destination size: {}, availabe size: {}",
                   sizeof(arg), rest));
    // arg = *(Arg *)(buf.data() + offset);
    std::copy(buf.data() + offset, buf.data() + offset + sizeof(arg),
              (char *)&arg);
    return _cast(buf, offset + sizeof(Arg), args...);
}

template <class Buffer, class... Args>
size_t cast(const Buffer &buf, Args &... args) {  // return the pos for next use
    return _cast(buf, 0, args...);
}

template <class Buffer>
void _build(Buffer &, const size_t) {
    return;
}

template <class Buffer, class Arg, class... Args>
void _build(Buffer &buf, const size_t offset, Arg &&arg, Args &&... args) {
    static_assert(std::is_standard_layout<tl_remove_cvref<Arg>>::value,
                  "Type should be standerd layout!");
    std::copy((char *)&arg, (char *)&arg + sizeof(arg), buf.data() + offset);
    _build(buf, offset + sizeof(arg), std::forward<Args>(args)...);
}

template <class Buffer, class... Args>
Buffer build(Args &&... args) {  // from POD to buf
    Buffer buf(variadic_sum(sizeof(args)...), 0);
    _build(buf, 0, std::forward<Args>(args)...);
    return buf;
}

}  // namespace message

_THALLIUM_END_NAMESPACE
#endif
