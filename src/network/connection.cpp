#include "connection.hpp"

#include <cstring>
#include <functional>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "asio_type_wrapper.hpp"
#include "common.hpp"
#include "heartbeat.hpp"
#include "logging.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
using namespace std::placeholders;
using asio_tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

string Connection::socket_to_string() const { return socket_str; }

Connection::Connection(execution_context &_context, asio_tcp::socket &&_socket,
                       const receive_callback &f, const event_callback &e_c)
    : _context(_context),
      holding_socket(new asio_tcp::socket{move(_socket)}),
      _receive_buffer(buffer_size),
      _header_buffer(),
      _write_queue(),
      socket_str(),
      upper_layer_callback(f),
      upper_layer_event_callback(e_c)

{
#ifdef DEBUG
    // for test, the heartbeat is send at short time, without delay will be
    // accumulated
    boost::asio::ip::tcp::no_delay option(true);
    holding_socket->set_option(option);
#endif
    socket_str = holding_socket->remote_endpoint().address().to_string() + ":" +
                 std::to_string(holding_socket->remote_endpoint().port());
}

void Connection::when_header_received(const std::error_code &ec, size_t) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            TI_DEBUG(format("Socket {}: Read canceled.", socket_to_string()));
        else if (ec.value() == asio::error::misc_errors::eof) {
            handle_eof();
        } else {
            TI_ERROR(format("Socket {}: Read error: {}", socket_to_string(),
                            ec.message()));
            connection_close();
            return;
        }
    } else {
        if(holding_socket->is_open()) //drop the msg if closed
            header_parser();
    }
}

void Connection::when_payload_received(const size_t length,
                                       const std::error_code &ec,
                                       const size_t bytes_read) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            TI_WARN(format(
                "Socket {}: Read canceled. But should not be cancelled here",
                socket_to_string()));
        else {
            TI_ERROR(format("Socket {}: Read error: {}", socket_to_string(),
                            ec.message()));
            connection_close();
            return;
        }
    }
    if (bytes_read < length) {
        TI_ERROR(
            format("Message body need: {} but only received: {}. Maybe the "
                   "peer: {} closes the connection",
                   length, bytes_read, socket_to_string()));
        connection_close();
        return;
    }

    if(holding_socket->is_open()){ // deal with the msg only when is open
        // send to upper layer
        upper_layer_callback(_receive_buffer.data(), length);
    }

    do_receive_message();
}

void Connection::do_receive_payload(const size_t length) {
    // double if buffer is not large to hold the body
    if (length > _receive_buffer.capacity()) {
        _receive_buffer.resize(_receive_buffer.capacity());
    }
    async_read(
        *holding_socket, asio::buffer(_receive_buffer, length),
        std::bind(&Connection::when_payload_received, this, length, _1, _2));
}

void Connection::do_receive_message() {
    // the close connection operation might called before here
    if (!holding_socket->is_open()) return;
    // receive header first
    async_read(*holding_socket, asio::buffer(_header_buffer),
               std::bind(&Connection::when_header_received, this, _1, _2));
}

void Connection::when_message_sent(const size_t length,
                                   const std::error_code &ec,
                                   const size_t bytes_write) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            TI_WARN(format(
                "Socket {}: Write canceled. But should not be cancelled here.",
                socket_to_string()));
        else {
            TI_ERROR(format("Socket {}: Write error: {}", socket_to_string(),
                            ec.message()));
            // write error close it
            connection_close();
            upper_layer_event_callback(message::ConnectionEvent::pipe);
            return;
        }
    }
    if (bytes_write <= length) {
        if (bytes_write == 0) {
            TI_ERROR(format("Message need to write: {} but only wrote: {}. ",
                            length, bytes_write, socket_to_string()));
            connection_close();
            return;
        }
    }

    // now can drop write buffer
    _write_queue.pop();
    // if remains in write buffer start anther async write
    if (_write_queue.size() > 0) do_write();
}

void Connection::do_write() {
    // fetch the back and call async write
    unique_ptr<buffer_sequence> &buf_seq_ptr = _write_queue.back();

    size_t total_size = 0;
    vector<asio::const_buffer> buffers_to_write;

    for (auto &zero_cpy_buf : *buf_seq_ptr) {
        total_size += zero_cpy_buf.size();
        buffers_to_write.push_back(asio::buffer(zero_cpy_buf.copyable_ref()));
    }

    std::error_code ec;
    async_write(
        *holding_socket, buffers_to_write,
        std::bind(&Connection::when_message_sent, this, total_size, _1, _2));
}

void Connection::do_send_header(message::ZeroCopyBuffer &&header_buf) {
    unique_ptr<buffer_sequence> buf_seq_ptr{new buffer_sequence};
    buf_seq_ptr->push_back(move(header_buf));

    _write_queue.push(move(buf_seq_ptr));
    if (_write_queue.size() == 1) {
        do_write();
    }
}

void Connection::do_send_message(message::ZeroCopyBuffer &&msg) {
    // not logic after send success
    auto length = msg.size();
    // generate normal header
    message::Header h{message::HeaderMessageType::normal,
                      static_cast<uint32_t>(length)};
    // fill the header_buf
    message::ZeroCopyBuffer tmp_header_buf(message::header_size, 0);
    message::header_to_string(h, tmp_header_buf.data());

    unique_ptr<buffer_sequence> buf_seq_ptr{new buffer_sequence};
    buf_seq_ptr->push_back(move(tmp_header_buf));
    buf_seq_ptr->push_back(move(msg));

    // https://stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-interleaving-async-write-calls
    _write_queue.push(move(buf_seq_ptr));
    // TODO hanlde maxium queue size?
    if (_write_queue.size() == 1) {
        do_write();
    }
}

void Connection::connection_close() {
    if (holding_socket->is_open()) {
        holding_socket->cancel();
        holding_socket->close();
    }
}

ServerConnection::ServerConnection(execution_context &_context,
                                   asio_tcp::socket &&s,
                                   const receive_callback &f,
                                   const event_callback &ec)
    : Connection(_context, move(s), f, ec) {}

void ServerConnection::header_parser() {
    message::Header h = message::string_to_header(_header_buffer.data());
    switch (h.msg_type) {
        case message::HeaderMessageType::normal:
            do_receive_payload(h.body_length);
            break;
        default:
            throw logic_error("Should not be here!");
    };
}

void ServerConnection::handle_eof() {
    TI_DEBUG(format("Client: {} closes connection.", socket_to_string()));
    connection_close();
    upper_layer_event_callback(message::ConnectionEvent::close);
}

ServerConnectionWithHeartbeat ::ServerConnectionWithHeartbeat(
    execution_context &_context, asio_tcp::socket &&s,
    const receive_callback &f, const event_callback &ec)
    : ServerConnection(_context, move(s), f, ec), HeartbeatChecker(_context) {
    heartbeat_start();
}

void ServerConnectionWithHeartbeat::header_parser() {
    message::Header h = message::string_to_header(_header_buffer.data());
    switch (h.msg_type) {
        case message::HeaderMessageType::heartbeat:
            // won't go here
            when_receive_heartbeat();
            do_receive_message();  // next header
            break;
        case message::HeaderMessageType::normal:
            do_receive_payload(h.body_length);
            break;
        default:
            throw logic_error("Should not be here!");
    };
}

void ServerConnectionWithHeartbeat::when_receive_heartbeat() {
    // now, set to initial timeout
    TI_DEBUG(format("Receive heartbeat from {}", socket_to_string()));
    heartbeat_received();
}

void ServerConnectionWithHeartbeat::connection_close() {
    heartbeat_stop();
    ServerConnection::connection_close();
}

void ServerConnectionWithHeartbeat::when_timeout(const std::error_code &ec) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            // could be the timer reset or stop the heartbeat
            return;
        else {
            TI_ERROR(format("Error when {} timer expires: {}",
                            socket_to_string(), ec.message()));
        }
    } else {
        TI_WARN(format("Peer socket: {} timeout", socket_to_string()));
        connection_close();
        upper_layer_event_callback(message::ConnectionEvent::timeout);
    }
}

ClientConnection::ClientConnection(execution_context &_context,
                                   asio_tcp::socket &&s,
                                   const receive_callback &f, const event_callback &ec)
    : Connection(_context, move(s), f, ec) {}

void ClientConnection::header_parser() {
    message::Header h = message::string_to_header(_header_buffer.data());
    switch (h.msg_type) {
        case message::HeaderMessageType::normal:
            size_t length = static_cast<size_t>(h.body_length);
            do_receive_payload(length);
            break;
            // TODO default
    };
}

void ClientConnection::handle_eof() {
    TI_ERROR(format("Server: {} closes connection.", socket_to_string()));
    connection_close();
    upper_layer_event_callback(message::ConnectionEvent::close);
}

ClientConnectionWithHeartbeat::ClientConnectionWithHeartbeat(
    execution_context &_context, asio_tcp::socket &&s,
    const receive_callback &f, const event_callback &ec)
    : ClientConnection(_context, move(s), f, ec), HeartbeatSender(_context) {
    heartbeat_start();
}

void ClientConnectionWithHeartbeat::send_heartbeat(const std::error_code &ec) {
    if (ec) {
        if (ec.value() == asio::error::operation_aborted)
            TI_DEBUG(format("Timer of socket id {}: {}", socket_to_string(),
                            ec.message()));
        else {
            TI_ERROR(format("Error when {} timer expires: {}",
                            socket_to_string(), ec.message()));
        }
        connection_close();
    } else {
        TI_DEBUG(format("Send heartbeat to {}.", socket_to_string()));
        message::Header h{message::HeaderMessageType::heartbeat, 0};
        message::ZeroCopyBuffer buf(message::header_size);
        message::header_to_string(h, buf.data());
        do_send_header(move(buf));
    }
}

void ClientConnectionWithHeartbeat::connection_close() {
    heartbeat_stop();
    ClientConnection::connection_close();
}

_THALLIUM_END_NAMESPACE
