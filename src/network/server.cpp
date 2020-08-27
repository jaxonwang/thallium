#include <functional>
#include <queue>
#include <system_error>
#include <type_traits>
#include <vector>

#include "logging.hpp"
#include "server.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
using namespace std::placeholders;
using asio_tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

ConnectionManager::ConnectionManager(execution_context &_context, const bool heartbeat)
    : _context(_context), holdings(), next_id(0), withheartbeat(heartbeat) {
  // TODO: periodically clean up closed connections
}

void ConnectionManager::new_connection(asio_tcp::socket &&peer) {
  // thread unsafe won't be called by multi threads
  int conn_id = next_id++;
  typedef Connection::receive_callback receive_callback;
  // add callback
  receive_callback f = bind(&ConnectionManager::receive, this, conn_id, _1, _2);
  ServerConnection * new_con_ptr;
  if(withheartbeat)
      new_con_ptr = new ServerConnectionWithHeartbeat(_context,move(peer), f);
  else
      new_con_ptr = new ServerConnection(_context, move(peer), f);
  holdings[conn_id] = unique_ptr<ServerConnection>{ new_con_ptr};
  holdings[conn_id]->do_receive_message();
}

void ConnectionManager::send(const int conn_id, message::ZeroCopyBuffer &&msg) {
  holdings[conn_id]->do_send_message(move(msg));
}

void ConnectionManager::receive(const int conn_id, const char *buf,
                                const size_t length) {
  upper->receive(conn_id, buf, length);
}

void ConnectionManager::disconnect(const int conn_id) {
  holdings[conn_id]->connection_close();
}

AsyncServer::AsyncServer(execution_context &ctx, const ti_socket_t &s, const bool heartbeat)
    : _socket(s), _context(ctx), _acceptor(_context), _cmanager(_context, heartbeat) {}

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
    do_accept(); // continue accept for next connection
  } else if (ec.value() == asio::error::operation_aborted) {
    TI_DEBUG("Acceptor stopped");
  } else {
    TI_WARN(format("Accept failed: {}", ec.message()));
  }
}

void AsyncServer::stop() {
  // cancel of connection is done by the logics in servermodel
  _acceptor.cancel();
  _acceptor.close();
}

void AsyncServer::start() {
  asio_tcp::endpoint ep{_socket.addr, _socket.port};
  _acceptor.open(ep.protocol());
  _acceptor.bind(ep);
  _acceptor.listen();
  // set local port
  if(_socket.port == 0)
      _socket.port = _acceptor.local_endpoint().port();

  // register callback
  do_accept();
}

void RunServer(ServerModel &s_impl, AsyncServer &s_async) {

  // need to compose these together
  // shit goes here
  s_impl.lower = &s_async._cmanager;
  s_impl.stopper = &s_async;
  s_impl.disconnector = &s_async._cmanager;

  s_async._cmanager.upper = &s_impl;

  s_async.start();
  s_impl.start();
}

_THALLIUM_END_NAMESPACE
