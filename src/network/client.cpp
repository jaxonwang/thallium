#include <boost/asio.hpp>
#include <memory>

#include "client.hpp"
#include "heartbeat.hpp"
#include "logging.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
using namespace std::placeholders;
using asio_tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

AsyncClient::AsyncClient(execution_context &ctx, const ti_socket_t &s)
    : _socket(s), _context(ctx) {}

void AsyncClient::start() {
    asio_tcp::endpoint ep{_socket.addr, _socket.port};
    asio_tcp::socket asio_s{_context};
    // why asio must accept endpoint list
    vector<asio_tcp::endpoint> eps;
    eps.push_back(ep);

    try {
        connect(asio_s, eps);
    } catch (const std::system_error &e) {
        TI_FATAL(format("Try to connect to {} failed: {}", _socket.to_string(),
                        e.what()));
        throw e;
    }
    TI_DEBUG(format("Successfully connect to {}", _socket.to_string()));
    // just return 0 as conn id
    function<void(const char *, const size_t)> f =
        std::bind(&AsyncClient::receive, this, 0, _1, _2);
    holding.reset(new ClientConnection(_context, move(asio_s), f));
}

ti_socket_t AsyncClient::client_socket() { return _socket; }

void AsyncClient::send(const int, message::ZeroCopyBuffer &&message) {
    holding->do_send_message(move(message));
}

void AsyncClient::receive(const int conn_id, const char *buf,
                          const size_t length) {
    upper->receive(conn_id, buf, length);
}

void AsyncClient::stop() {}

_THALLIUM_END_NAMESPACE
