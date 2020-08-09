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
    ZeroCopyBuffer() : CopyableBuffer(){};
    ZeroCopyBuffer(size_type, value_type = size_type());
    ZeroCopyBuffer(const ZeroCopyBuffer &) = delete;
    ZeroCopyBuffer(CopyableBuffer &&);
    ZeroCopyBuffer(ZeroCopyBuffer &&);
    ZeroCopyBuffer &operator=(const ZeroCopyBuffer &) = delete;
    CopyableBuffer &to_copyable();
};

class ReadOnlyBuffer {
    // caution! this does not manage the life time of under-lying buffer!
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

inline void _rest_at_least(size_t rest, size_t at_least) {
    if (rest < at_least)
        throw std::runtime_error(
            format("bad cast: destination size: {}, availabe size: {}",
                   at_least, rest));
}

template <class Buffer, class Arg>
size_t _cast_one(const Buffer &buf, const size_t offset, Arg &arg) {
    static_assert(std::is_standard_layout<tl_remove_cvref<Arg>>::value,
                  "Type should be standerd layout!");
    _rest_at_least(buf.size() - offset, sizeof(arg));

    const char *start = buf.data() + offset;
    std::copy(start, start + sizeof(arg), (char *)&arg);
    return offset + sizeof(Arg);
}

template <class Buffer>
size_t _cast_one(const Buffer &buf, const size_t offset, ZeroCopyBuffer &varlen) {
    // will introduce a copy, manually cast to a var len
    // type if we want to avoid the copy;
    _rest_at_least(buf.size() - offset, sizeof(u_int32_t));
    u_int32_t len32;
    size_t next = _cast_one(buf, offset, len32);

    size_t length = static_cast<size_t>(len32);
    _rest_at_least(buf.size() - offset, length + sizeof(u_int32_t));
    const char * start = buf.data() + next;

    varlen.resize(length);
    std::copy(start, start + length, varlen.data());
    return offset + sizeof(u_int32_t) + length;
}

template <class Buffer>
size_t _cast(const Buffer &, const size_t offset) {
    return offset;
}

template <class Buffer, class Arg, class... Args>
size_t _cast(const Buffer &buf, const size_t offset, Arg &arg, Args &... args) {
    size_t next_offset = _cast_one(buf, offset, arg);
    return _cast(buf, next_offset, args...);
}

template <class Buffer, class... Args>
size_t cast(const Buffer &buf, Args &... args) {  // return the pos for next use
    return _cast(buf, 0, args...);
}

template <class Buffer, class Arg>
size_t _build_one(Buffer &buf, const size_t offset, const Arg &arg){
    static_assert(std::is_standard_layout<tl_remove_cvref<Arg>>::value,
                  "Type should be standerd layout!");
    std::copy((char *)&arg, (char *)&arg + sizeof(arg), buf.data() + offset);
    return offset + sizeof(arg);
}

template <class Buffer>
size_t _build_one(Buffer &buf, const size_t offset, const ReadOnlyBuffer &varlen){
    u_int32_t len32 = static_cast<u_int32_t>(varlen.size());
    size_t next_offset = _build_one(buf, offset, len32);
    std::copy(varlen.data(), varlen.data() + varlen.size(),
              buf.data() + next_offset);
    next_offset += varlen.size();
    return next_offset;
}

template <class Buffer, class Arg, class... Args>
void _build(Buffer &buf, const size_t offset, Arg &&arg, Args &&... args) {
    size_t next_offset = _build_one(buf, offset, arg);
    _build(buf, next_offset, std::forward<Args>(args)...);
}

template <class Buffer>
void _build(Buffer &, const size_t) {
    return;
}

template <class T>
inline size_t _type_size(const T & t){
    return sizeof(t);
}

inline size_t _type_size(const ReadOnlyBuffer &buf){
    return sizeof(u_int32_t) + buf.size(); // with 4 bytes to specify length
}

template <class Buffer, class... Args>
Buffer build(Args &&... args) {  // from POD to buf
    Buffer buf(variadic_sum(_type_size(args)...), 0);
    _build(buf, 0, std::forward<Args>(args)...);
    return buf;
}

}  // namespace message

_THALLIUM_END_NAMESPACE
#endif
