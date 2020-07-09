#include <boost/asio.hpp>
#include <csignal>
#include <cstring>
#include <functional>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "logging.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "heartbeat.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
using namespace std::placeholders;
using asio_tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

class ConnectionManager {
    typedef std::unique_ptr<asio_tcp::socket> socket_ptr;
    typedef std::unique_ptr<asio::steady_timer> timer_ptr;

  private:
    const static size_t buffer_size = 8192;
    execution_context &_context;
    unordered_map<int, socket_ptr> holdings;
    unordered_map<int, timer_ptr> heart_beat_timers;
    // life time of context is longer than connection manager
    // this buffer will only grow but never shinks
    vector<char> _receive_buffer;
    array<char, message::header_size> _header_buffer;
    int next_id;

  public:
    ConnectionManager(execution_context &_context);
    ConnectionManager(const ConnectionManager &c) = delete;

    void new_connection(asio_tcp::socket &&s);
    void when_timeout(const int conn_id, const std::error_code &ec);
    void server_side_close(const int conn_id);
    void when_receive_heart_beat(const int);
    void do_receive_message(const int conn_id);
    void when_header_received(const int, const std::error_code &, const size_t);
    void do_receive_payload(const int conn_id, const size_t length);
    void when_payload_received(const int, const size_t, const std::error_code &,
                               const size_t);
    string socket_to_string(const int conn_id);
};

class AsyncServer : public Server {
  public:
    AsyncServer(const ti_socket_t &);
    void start() override;
    void stop() override;
    ti_socket_t server_socket() override;

  private:
    ti_socket_t _socket;
    execution_context _context;  // mystrious context used by asio
    asio_tcp::acceptor _acceptor;
    thallium::signal_set _signals;
    ConnectionManager _cmanager;

    void when_accept(const std::error_code &ec, asio_tcp::socket &&peer);
    void do_accept();
};

string ConnectionManager::socket_to_string(const int conn_id) {
    auto s = holdings[conn_id].get();
    return s->remote_endpoint().address().to_string() + ":" +
           std::to_string(s->remote_endpoint().port());
}

ConnectionManager::ConnectionManager(execution_context &_context)
    : _context(_context),
      holdings(),
      heart_beat_timers(),
      _receive_buffer(buffer_size),
      _header_buffer(),
      next_id(0) {}

void ConnectionManager::new_connection(asio_tcp::socket &&peer) {
    // thread unsafe won't be called by multi threads
    int conn_id = next_id++;
    holdings[conn_id] = socket_ptr{new asio_tcp::socket(move(peer))};

    // set timer for heart beat now
    heart_beat_timers[conn_id] =
        timer_ptr{new asio::steady_timer(_context, HeartBeatPolicy::timeout)};
    heart_beat_timers[conn_id]->async_wait(
        std::bind(&ConnectionManager::when_timeout, this, conn_id, _1));
    // TODO change states in place session manager
    do_receive_message(conn_id);
}

void ConnectionManager::when_header_received(const int conn_id,
                                         const std::error_code &ec,
                                         size_t bytes_read) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            TI_DEBUG(
                format("Socket {}: Read canceled.", socket_to_string(conn_id)));
        else {
            TI_DEBUG(format("Socket {}: Read error: {}",
                            socket_to_string(conn_id), ec.message()));
            return;
        }
    }
    if (bytes_read == 0) {
        TI_DEBUG(format("Peer {} closed.", socket_to_string(conn_id)));
        server_side_close(conn_id);
    }
    message::Header h = message::string_to_header(_header_buffer.data());
    switch (h.msg_type) {
        case message::HeaderMessageType::heartbeat:
            // reset timer
            when_receive_heart_beat(conn_id);
            do_receive_message(conn_id);  // next header
            break;
        case message::HeaderMessageType::normal:
            size_t length = static_cast<size_t>(h.body_length);
            // double if buffer is not large to hold the body
            if (length > _receive_buffer.capacity()) {
                _receive_buffer.resize(_receive_buffer.capacity());
            }
            do_receive_payload(conn_id, length);
            break;
            // TODO default
    };
}

void ConnectionManager::when_payload_received(const int conn_id,
                                              const size_t length,
                                              const std::error_code &ec,
                                              const size_t bytes_read) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            TI_WARN(
                format("Socket {}: Read canceled. But should not cancel here",
                       socket_to_string(conn_id)));
        else {
            TI_DEBUG(format("Socket {}: Read error: {}",
                            socket_to_string(conn_id), ec.message()));
            return;
        }
    }
    if (bytes_read <= length) {
        if (bytes_read == 0) {
            TI_ERROR(
                format("Message body need: {} but only received: {}. Since the "
                       "peer: {} closes the connection",
                       length, bytes_read, socket_to_string(conn_id)));
            server_side_close(conn_id);
            return;
        }
    }
    // TODO: now should call the real server
    //
    // receive next msg
    do_receive_message(conn_id);
}

void ConnectionManager::do_receive_payload(const int conn_id,
                                           const size_t length) {
    auto condition = [=](const std::error_code &, size_t bytes_read) -> size_t {
        // I assume if there is a error asio will handle it correctly even if i
        // return > 0 here
        return length - bytes_read;
    };
    async_read(*holdings[conn_id], asio::buffer(_receive_buffer), condition,
               std::bind(&ConnectionManager::when_payload_received, this, conn_id,
                         length, _1, _2));
}

void ConnectionManager::do_receive_message(const int conn_id) {
    // receive header first
    async_read(
        *holdings[conn_id], asio::buffer(_header_buffer),
        std::bind(&ConnectionManager::when_header_received, this, conn_id, _1, _2));
}

void ConnectionManager::when_receive_heart_beat(const int conn_id) {
    heart_beat_timers[conn_id]->expires_from_now(HeartBeatPolicy::timeout);
}

void ConnectionManager::server_side_close(const int conn_id) {
    heart_beat_timers[conn_id]->cancel();
    holdings[conn_id]->close();
    holdings.erase(conn_id);
}

void ConnectionManager::when_timeout(const int conn_id,
                                     const std::error_code &ec) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            TI_DEBUG(format("Timer of socket id {}: {}",
                            socket_to_string(conn_id), ec.message()));
        else {
            TI_ERROR(format("Error when {} timer expires: {}",
                            socket_to_string(conn_id), ec.message()));
        }
    } else {
        TI_WARN(format("Peer socket: {} timeout", socket_to_string(conn_id)));
        server_side_close(conn_id);
    }
}

AsyncServer::AsyncServer(const ti_socket_t &s)
    : _socket(s),
      _context(1),
      _acceptor(_context),
      _signals(_context),
      _cmanager(_context) {
#if defined(SIGPIPE)
    _signals.add(SIGPIPE);
#endif
    // just ignore signals
    _signals.async_wait([](const std::error_code &, int) {});

    asio_tcp::endpoint ep{s.addr, s.port};
    _acceptor.open(ep.protocol());
    _acceptor.bind(ep);
    _acceptor.listen();

    // register callback
    do_accept();
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

void AsyncServer::start() { _context.run(); }

std::shared_ptr<Server> ServerFactory(const ti_socket_t &s) {
    return make_shared<AsyncServer>(s);
}

_THALLIUM_END_NAMESPACE
