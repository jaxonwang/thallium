#include "runtime_protocol.hpp"

#include <random>

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 255);

constexpr size_t FirstConCookie::data_len;

void bad_message() { throw runtime_error{"bad message format!"}; }

void length_assert(const size_t expected, const size_t length) {
    if (expected != length) bad_message();
}

MessageType read_header_messagetype(const message::ReadOnlyBuffer& buf) {
    runtime_header header;
    message::cast(buf, header);
    return header.type;
}

FirstConCookie::FirstConCookie() {
    for (size_t i = 0; i < data_len; i++) {
        data[i] = dis(gen);  // TODO: security
    }
}
FirstConCookie::FirstConCookie(const std::string& printable) {
    if (printable.size() != data_len * 2) {
        throw runtime_error(
            format("The cookie string length should be {}", 2 * data_len));
    }
    for (size_t i = 0; i < data_len * 2; i += 2) {
        char c_byte_h = printable[i];
        char c_byte_l = printable[i + 1];
        auto ctoi = [](char c_byte) {
            u_int8_t byte;
            if (c_byte >= '0' && c_byte <= '9')
                byte = c_byte - '0';
            else if (c_byte >= 'A' && c_byte <= 'F')
                byte = c_byte - 'A' + 10;
            else
                throw runtime_error("string element should be hexdecimal");
            return byte;
        };
        data[i / 2] = ctoi(c_byte_h) * 16 + ctoi(c_byte_l);
    }
}

FirstConCookie::FirstConCookie(const char* buf, const size_t length) {
    length_assert(length, data_len);
    for (size_t i = 0; i < length; i++) {
        data[i] = buf[i];
    }
}

string FirstConCookie::to_printable() const {
    string a(data_len * 2, '0');
    static const char* digits = "0123456789ABCDEF";
    for (size_t i = 0; i < data_len; i++) {
        a[2 * i] = digits[data[i] / 16];
        a[2 * i + 1] = digits[data[i] % 16];
    }
    return a;
}

bool FirstConCookie::operator==(const FirstConCookie& other) const {
    for (size_t i = 0; i < data_len; i++) {
        if (data[i] != other.data[i]) return false;
    }
    return true;
}

bool operator!=(const FirstConCookie a, const FirstConCookie& b) {
    return !(a == b);
}

Firsconnection::Firsconnection(const string& cookie) : firstcookie(cookie) {}

Firsconnection::Firsconnection(const FirstConCookie cookie)
    : firstcookie(cookie) {}

Firsconnection Firsconnection::from_buffer(const message::ReadOnlyBuffer& buf) {
    runtime_header header;
    FirstConCookie cookie;
    auto ret = message::cast(buf, header, cookie.data);
    length_assert(ret, buf.size());
    return Firsconnection(move(cookie));
}

message::ZeroCopyBuffer Firsconnection::to_buffer() const {
    return message::build<message::ZeroCopyBuffer>(runtime_header{MessageType::firstconnection},
            firstcookie.data);
}

message::ZeroCopyBuffer FirsconnectionOK::to_buffer() const {
    return message::build<message::ZeroCopyBuffer>(runtime_header{MessageType::firstconnectionok});
}

_THALLIUM_END_NAMESPACE
