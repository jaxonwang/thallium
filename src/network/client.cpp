#include "client.hpp"

#include <boost/asio.hpp>
#include <memory>

#include "heartbeat.hpp"
#include "logging.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
using namespace std::placeholders;
using asio_tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

template<class Endpoints>
real_addr_type AsyncClient::try_connect(asio_tcp::socket & asio_s, Endpoints & eps){

    decltype(connect(asio_s, eps)) ep;
    try {
        ep = connect(asio_s, eps);
        // copy the connected
    } catch (const std::system_error &e) {
        TI_FATAL(format("Connect failed: {}",
                        e.what()));
        throw e;
    }
    TI_DEBUG(format("Successfully connect to {}", ep.address().to_string()));

    // just return 0 as conn id
    function<void(const char *, const size_t)> f =
        std::bind(&AsyncClient::receive, this, 0, _1, _2);
    holding.reset(new ClientConnection(_context, move(asio_s), f));

    return ep.address().to_v4();
}

// will throw
AsyncClient::AsyncClient(execution_context &ctx, const ti_socket_t &s)
    : _socket(s), _context(ctx) {
    asio_tcp::endpoint ep{_socket.addr, _socket.port};
    asio_tcp::socket asio_s{_context};
    // why asio must accept endpoint list
    vector<asio_tcp::endpoint> eps;
    eps.push_back(ep);

    try_connect(asio_s, eps);
}

AsyncClient::AsyncClient(execution_context &ctx, const string &hostname,
                         unsigned short port)
    : _context(ctx) {
    asio_tcp::resolver rslver{_context};

    typename asio_tcp::resolver::results_type eps;

    try {
        eps = rslver.resolve(hostname, to_string(port));
    } catch (const std::system_error &e) {
        TI_FATAL(format("Unable to resolve {}, error: {}", hostname, e.what()));
    }

    asio_tcp::socket asio_s{_context};
    _socket.addr = try_connect(asio_s, eps);
    _socket.port = port;
}

void AsyncClient::start() { holding->do_receive_message(); }

ti_socket_t AsyncClient::client_socket() { return _socket; }

void AsyncClient::send(const int, message::ZeroCopyBuffer &&message) {
    holding->do_send_message(move(message));
}

void AsyncClient::receive(const int conn_id, const char *buf,
                          const size_t length) {
    upper->receive(conn_id, buf, length);
}

void AsyncClient::disconnect(const int) { holding->connection_close(); }

void AsyncClient::stop() {}

void RunClient(ClientModel &c_impl, AsyncClient &c_async) {
    // need to compose these together
    // shit goes here
    c_impl.lower = &c_async;
    c_impl.stopper = &c_async;
    c_impl.disconnector = &c_async;

    c_async.upper = &c_impl;

    c_async.start();
    c_impl.start();
}

_THALLIUM_END_NAMESPACE
