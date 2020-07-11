#include <functional>
#include <system_error>
#include <type_traits>
#include <vector>
#include <queue>


#include "server.hpp"
#include "logging.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
using namespace std::placeholders;
using asio_tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

ConnectionManager::ConnectionManager(execution_context &_context)
    : _context(_context),
      holdings(),
      next_id(0) {}

void ConnectionManager::new_connection(asio_tcp::socket &&peer) {
    // thread unsafe won't be called by multi threads
    int conn_id = next_id++;
    typedef Connection::receive_callback receive_callback;
    // add callback
    receive_callback f = bind(&ConnectionManager::receive, this, conn_id, _1, _2);
    holdings[conn_id] = unique_ptr<ServerConnection>{new ServerConnection(_context, move(peer), f)};
    holdings[conn_id]->do_receive_message();
}

void ConnectionManager::send(const int conn_id, message::ZeroCopyBuffer &&msg){
    holdings[conn_id]->do_send_message(move(msg));
}

void ConnectionManager::receive(const int conn_id, const char * buf, const size_t length){
    upper->receive(conn_id, buf, length);
}

AsyncServer::AsyncServer(execution_context &ctx, const ti_socket_t &s)
    : _socket(s), _context(ctx), _acceptor(_context), _cmanager(_context) {}

ti_socket_t AsyncServer::server_socket() { return _socket; }

void AsyncServer::do_accept() {
    _acceptor.async_accept(std::bind(&AsyncServer::when_accept, this, _1, _2));
}

void AsyncServer::when_accept(const std::error_code &ec,
                              asio_tcp::socket &&peer) {
    if (!ec) {
        TI_DEBUG(format("Accepting connection from {}",
                        peer.remote_endpoint().address().to_string()));
        _cmanager.new_connection(move(peer));
        do_accept();  // continue accept for next connection
    } else if (ec.value() == asio::error::operation_aborted) {
        TI_DEBUG("Acceptor stopped");
    } else {
        TI_WARN(format("Accept failed: {}", ec.message()));
    }
}

void AsyncServer::stop() {
    _acceptor.cancel();
    _acceptor.close();
}

void AsyncServer::start() {
    asio_tcp::endpoint ep{_socket.addr, _socket.port};
    _acceptor.open(ep.protocol());
    _acceptor.bind(ep);
    _acceptor.listen();

    // register callback
    do_accept();
}

_THALLIUM_END_NAMESPACE
