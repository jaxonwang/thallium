#ifndef _THALLIUM_NETWORK_HEADER
#define _THALLIUM_NETWORK_HEADER

#include "common.hpp"
#include "asio_type_wrapper.hpp"

_THALLIUM_BEGIN_NAMESPACE

bool valid_hostname(const char *);

bool valid_port(const int);

int port_to_int(const char *);

struct socket_holder{
    int port;
    real_addr_type addr;
};

struct Message{
    socket_holder s;
};

class Server {
  public:
    Server(socket_holder &s);
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    void operator=(Server &&) = delete;
    void operator=(const Server &) = delete;

    void start();

    socket_holder _socket;
  private:
    execution_context _context; // mystrious context used by asio
    thallium::signal_set _signals;
};

_THALLIUM_END_NAMESPACE
#endif
