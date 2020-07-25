#ifndef _THALLIUM_RUNTIME_PROTOCOL
#define _THALLIUM_RUNTIME_PROTOCOL

#include <functional>

#include "common.hpp"
#include "network/network.hpp"

_THALLIUM_BEGIN_NAMESPACE

enum MessageType : u_int8_t {
    firstconnection,
    firstconnectionok,
    peersinfo,
    peersconnected,
    fail,
};

struct runtime_header{
    MessageType type;
};

MessageType read_header_messagetype(const message::ReadOnlyBuffer&);

class FirstConCookie {
  public:
    constexpr static size_t data_len = 16;
    u_int8_t data[data_len];
    FirstConCookie();
    FirstConCookie(const std::string& printable);
    FirstConCookie(const char* buf, const size_t length);
    std::string to_printable();
    bool operator==(const FirstConCookie& other) const;
};

bool operator!=(const FirstConCookie a, const FirstConCookie& b);

class Message {
    virtual message::ZeroCopyBuffer to_buffer() const = 0;
};

class Firsconnection : public Message {
    public:
    FirstConCookie firstcookie;
    Firsconnection(const FirstConCookie cookie);
    Firsconnection(const std::string& cookie);
    message::ZeroCopyBuffer to_buffer() const override;
    static Firsconnection from_buffer(const message::ReadOnlyBuffer &);
};

class FirsconnectionOK: public Message{
    message::ZeroCopyBuffer to_buffer() const override;
};

_THALLIUM_END_NAMESPACE

namespace std {

template <>
struct hash<thallium::FirstConCookie> {
    std::size_t operator()(const thallium::FirstConCookie& c) const noexcept {
        return *(std::size_t*)&c.data;  // or use boost::hash_combine
    }
};
}  // namespace std
#endif
