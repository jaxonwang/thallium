#include <boost/asio.hpp>
#include <csignal>
#include <cstring>
#include <vector>
#include <system_error>

#include "common.hpp"
#include "network.hpp"

_THALLIUM_BEGIN_NAMESPACE

class AsyncServer {
  public:
    AsyncServer();
    AsyncServer(const AsyncServer &) = delete;
    AsyncServer(AsyncServer &&) = delete;
    void operator=(AsyncServer &&) = delete;
    void operator=(const AsyncServer &) = delete;
    void start();
    int port;

  private:
};

Server::Server(socket_holder &s) : _socket(s), _context(1), _signals(_context) {
#if defined(SIGPIPE)
    _signals.add(SIGPIPE);
#endif
    // just ignore signals
    _signals.async_wait([](const boost::system::error_code &, int){});


}
void Server::start() {}

_THALLIUM_END_NAMESPACE
