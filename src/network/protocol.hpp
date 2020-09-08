#ifndef _THALLIUM_NETWORK_PROTOCOL_HEADER
#define _THALLIUM_NETWORK_PROTOCOL_HEADER

#include <memory>
#include <type_traits>
#include <vector>

#include "common.hpp"
#include "serialize.hpp"
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

enum class ConnectionEvent : u_int8_t {
    timeout,
    close,  // in current design, if server close proactively, server will still
            // receive such event, caused by impl of ASIO
    pipe, // when write to a closed fd
    start, // only use by client, to run the initial sending message in the main loop
};

typedef std::vector<char> CopyableBuffer;

class ZeroCopyBuffer : public CopyableBuffer {
  public:
    ZeroCopyBuffer() : CopyableBuffer(){};
    ZeroCopyBuffer(size_type, value_type = 0);
    ZeroCopyBuffer(const ZeroCopyBuffer &) = delete;
    ZeroCopyBuffer(CopyableBuffer &&);
    ZeroCopyBuffer(ZeroCopyBuffer &&);
    ZeroCopyBuffer &operator=(const ZeroCopyBuffer &) = delete;
    CopyableBuffer &to_copyable();
    // note: can directly be serializable because it is a vector, otherwise need
    // to rewrite this
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
    template <class Archive>
    void serializable(Archive &ar) {
        auto sp = gsl::span<const char>(buf, buflen);
        ar &sp;
    }
};

inline void _rest_at_least(size_t rest, size_t at_least) {
    if (rest < at_least)
        throw std::runtime_error(
            format("bad cast: destination size: {}, availabe size: {}",
                   at_least, rest));
}
}  // namespace message

namespace Serializer {

template <class Buffer>
class FixSizeBufferSavingArchive
    : public SaveArchive<FixSizeBufferSavingArchive<Buffer>> {
  private:
    Buffer &_buf;
    const size_t _buf_size;
    char *buf_begin;
    char *save_cursor;

  public:
    FixSizeBufferSavingArchive(Buffer &buf)
        : _buf(buf),
          _buf_size(_buf.size()),
          buf_begin(_buf.data()),
          save_cursor(buf_begin) {}
    char *get_save_cursor(const size_t num) {
        char *ret = save_cursor;
        char *result = save_cursor + num;
        if (result > _buf_size + buf_begin) {
            throw std::runtime_error(format(
                "Internal buf not big enough to hold {} size value.", num));
        }
        save_cursor = result;
        return ret;
    }
    size_t size() const { return save_cursor - buf_begin; }
};

template <class Buffer>
class FixSizeBufferLoadingArchive
    : public LoadArchive<FixSizeBufferLoadingArchive<Buffer>> {
  private:
    const Buffer &_buf;
    const size_t _buf_size;
    const char *buf_begin;
    const char *load_cursor;

  public:
    FixSizeBufferLoadingArchive(const Buffer &buf)
        : _buf(buf),
          _buf_size(_buf.size()),
          buf_begin(_buf.data()),
          load_cursor(buf_begin) {}
    const char *get_load_cursor(size_t num) {
        const char *ret = load_cursor;
        const char *result = load_cursor + num;
        if (result > _buf_size + buf_begin) {
            throw std::runtime_error(format(
                "Remaining buf (size: {}) not enough to convert {} size value.",
                _buf_size + buf_begin - load_cursor, num));
        }
        load_cursor = result;

        return ret;
    }
    size_t size() const { return load_cursor - buf_begin; }
};

}  // namespace Serializer

namespace message {

template <class Archive>
size_t _cast(Archive &ar) {
    return ar.size();
}

template <class Archive, class Arg, class... Args>
size_t _cast(Archive &ar, Arg &arg, Args &... args) {
    ar >> arg;
    return _cast(ar, args...);
}

template <class Buffer, class... Args>
size_t cast(const Buffer &buf, Args &... args) {  // return the pos for next use
    Serializer::FixSizeBufferLoadingArchive<Buffer> arcv{buf};
    return _cast(arcv, args...);
}

template <class Archive>
void _build(Archive &) {
    return;
}

template <class Archive, class Arg, class... Args>
void _build(Archive &ar, Arg &&arg, Args &&... args) {
    ar << arg;
    _build(ar, std::forward<Args>(args)...);
}

template <class Buffer, class... Args>
Buffer build(Args &&... args) {  // from POD to buf
    size_t total = variadic_sum(Serializer::real_size(args)...);
    Buffer buf(total, 0);
    Serializer::FixSizeBufferSavingArchive<Buffer> arcv{buf};
    _build(arcv, std::forward<Args>(args)...);
    return buf;
}

}  // namespace message

_THALLIUM_END_NAMESPACE
#endif
