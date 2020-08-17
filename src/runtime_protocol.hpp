#ifndef _THALLIUM_RUNTIME_PROTOCOL
#define _THALLIUM_RUNTIME_PROTOCOL

#include <functional>

#include "common.hpp"
#include "network/network.hpp"
#include "serialize.hpp"

_THALLIUM_BEGIN_NAMESPACE

enum MessageType : u_int8_t {
    firstconnection,
    firstconnectionok,
    peersinfo,
    peersconnected,
    fail,
};

struct runtime_header {
    MessageType type;
    template <class Archive>
    void serializable(Archive& ar) {
        ar& type;
    }
};

MessageType read_header_messagetype(const message::ReadOnlyBuffer&);

class FirstConCookie {
  public:
    constexpr static size_t data_len = 16;
    u_int8_t data[data_len];
    FirstConCookie();
    explicit FirstConCookie(const std::string& printable);
    FirstConCookie(const char* buf, const size_t length);
    std::string to_printable() const;
    bool operator==(const FirstConCookie& other) const;
};

bool operator!=(const FirstConCookie a, const FirstConCookie& b);

class Message {
    virtual message::ZeroCopyBuffer to_buffer() const = 0;
};

template <class MsgClass, MessageType msg_type_header, class... Args>
struct Marshaller {
    template <Args... args>
    struct with {
        static MsgClass from_buffer(const message::ReadOnlyBuffer& buf) {
            MsgClass msgobj;
            runtime_header header;
            message::cast(buf, header, (msgobj.*args)...);
            return msgobj;
        }
        static message::ZeroCopyBuffer to_buffer(const MsgClass& msgobj) {
            auto header = runtime_header{msg_type_header};
            return message::build<message::ZeroCopyBuffer>(header,
                                                           (msgobj.*args)...);
        }
    };
};

template <class MsgClass>
MsgClass from_buffer(const message::ReadOnlyBuffer& buf) {
    return MsgClass::marshaller::from_buffer(buf);
}

template <class MsgClass>
message::ZeroCopyBuffer to_buffer(const MsgClass& msgobj) {
    return MsgClass::marshaller::to_buffer(msgobj);
}

class Firsconnection : public Message {
  public:
    FirstConCookie firstcookie;
    Firsconnection(const FirstConCookie cookie);
    Firsconnection(const std::string& cookie);
    message::ZeroCopyBuffer to_buffer() const override;
    static Firsconnection from_buffer(const message::ReadOnlyBuffer&);
};

class FirsconnectionOK : public Message {
  public:
    using marshaller =
        Marshaller<FirsconnectionOK, MessageType::firstconnectionok>::with<>;
    message::ZeroCopyBuffer to_buffer() const override;
};

class Peerinfo : public Message {
  public:
    std::string address;
    unsigned short port;
    Peerinfo(const std::string& address, const unsigned short port);
    message::ZeroCopyBuffer to_buffer() const override;
    static Firsconnection from_buffer(const message::ReadOnlyBuffer&);
};

_THALLIUM_END_NAMESPACE

namespace std {

template <>
struct hash<thallium::FirstConCookie> {
    std::size_t operator()(const thallium::FirstConCookie& c) const noexcept {
        union {
            size_t s;
            u_int8_t bytes[sizeof(size_t)];
        } ret{};
        std::copy(c.data, c.data + sizeof(size_t), ret.bytes);
        return ret.s;
    }
};
}  // namespace std

#endif
