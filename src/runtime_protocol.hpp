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
    MessageType message_type;
    template <class Archive>
    void serializable(Archive& ar) {
        ar& message_type;
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
    template <class Archive>
    void serializable(Archive& ar) {
        ar& data;
    }
};

bool operator!=(const FirstConCookie a, const FirstConCookie& b);

class Message {
};

template <class MsgClass>
MsgClass from_buffer(const message::ReadOnlyBuffer& buf) {
    MsgClass msg;
    runtime_header header;
    message::cast<message::ReadOnlyBuffer>(buf, header, msg);
    if (header.message_type != MsgClass::message_type) {
        throw std::logic_error("Message type unmatch!");
    }
    return msg;
}

template <class MsgClass>
message::ZeroCopyBuffer to_buffer(const MsgClass& msgobj) {
    runtime_header header{MsgClass::message_type};
    return message::build<message::ZeroCopyBuffer>(header, msgobj);
}

class Firsconnection : public Message {
  public:
    FirstConCookie firstcookie;
    Firsconnection() = default;
    Firsconnection(const FirstConCookie cookie);
    Firsconnection(const std::string& cookie);
    constexpr static MessageType message_type = MessageType::firstconnection;
    template <class Archive>
    void serializable(Archive& ar) {
        ar& firstcookie;
    }
};

class FirsconnectionOK : public Message {
  public:
    constexpr static MessageType message_type = MessageType::firstconnectionok;
    template <class Archive>
    void serializable(Archive&) {}
};

class Peerinfo : public Message {
  public:
    std::string address;
    unsigned short port;
    Peerinfo(const std::string& address, const unsigned short port);
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
