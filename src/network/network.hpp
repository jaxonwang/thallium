#ifndef _THALLIUM_NETWORK_HEADER
#define _THALLIUM_NETWORK_HEADER

#include <memory>
#include <string>

#include "asio_type_wrapper.hpp"
#include "common.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

// in network utils

bool valid_hostname(const char *);

bool valid_port(const int);

int port_to_int(const char *);

// in asio_type_wrapper

struct ti_socket_t {
    unsigned short port;
    real_addr_type addr;
    std::string to_string() const;
};


// in server.hpp

class Server {
  public:
    Server() = default;
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    void operator=(Server &&) = delete;
    void operator=(const Server &) = delete;
    virtual ~Server() = default;
    virtual ti_socket_t server_socket() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

class Client {
  public:
    Client() = default;
    Client(const Client &) = delete;
    Client(Client &&) = delete;
    void operator=(Client &&) = delete;
    void operator=(const Client &) = delete;
    virtual ~Client() = default;
    virtual ti_socket_t client_socket() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

_THALLIUM_END_NAMESPACE
#endif
