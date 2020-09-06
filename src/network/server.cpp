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

AsyncServer::AsyncServer(execution_context &ctx, const ti_socket_t &s, const bool heartbeat)
    : _socket(s), _context(ctx), _acceptor(_context) {
        if(heartbeat)
            _cmanager.reset(new RealConnectionManager<ServerConnectionWithHeartbeat>{_context});
        else
            _cmanager.reset(new RealConnectionManager<ServerConnection>{_context});
    }

ti_socket_t AsyncServer::server_socket() { return _socket; }

void AsyncServer::do_accept() {
  _acceptor.async_accept(std::bind(&AsyncServer::when_accept, this, _1, _2));
}

void AsyncServer::when_accept(const std::error_code &ec,
                              asio_tcp::socket &&peer) {
  if (!ec) {
    TI_DEBUG(format("Accepting connection from {}",
                    peer.remote_endpoint().address().to_string()));
    _cmanager->new_connection(move(peer));
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
  s_impl.lower = s_async._cmanager.get();
  s_impl.stopper = &s_async;
  s_impl.disconnector = s_async._cmanager.get();

  s_async._cmanager->set_upper(&s_impl);

  s_async.start();
  s_impl.start();
}

_THALLIUM_END_NAMESPACE
