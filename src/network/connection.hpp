#ifndef _THALLIUM_NETWORK_CONNECTION
#define _THALLIUM_NETWORK_CONNECTION

#include <boost/asio.hpp>
#include <functional>
#include <queue>

#include "asio_type_wrapper.hpp"
#include "common.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

typedef std::unique_ptr<boost::asio::steady_timer> timer_ptr;

class Connection {
  public:
    // use char * to show that the the data in char * will be availabe 
    // during the whole callback execution, with the guarantee that connection is
    // singal thread read
    typedef std::function<void(const char*, size_t)> receive_callback;

  protected:
    using asio_tcp = boost::asio::ip::tcp;
    typedef std::unique_ptr<asio_tcp::socket> socket_ptr;
    typedef std::vector<message::ZeroCopyBuffer> buffer_sequence;

    const static size_t buffer_size = 8192;
    execution_context &_context;
    socket_ptr holding_socket;
    timer_ptr heartbeat_timer;
    // this buffer will only grow but never shinks
    std::vector<char> _receive_buffer;
    std::array<char, message::header_size> _header_buffer;
    std::queue<std::unique_ptr<buffer_sequence>> _write_queue;
    std::string socket_str;

    receive_callback upper_layer_callback;
    void do_write();

  public:
    Connection(execution_context &_context, asio_tcp::socket &&s,
               const receive_callback &f, const std::chrono::seconds timeout);
    Connection(const Connection &c) = delete;
    virtual ~Connection() = default;

    void connection_close();
    void do_receive_message();
    void when_header_received(const std::error_code &, const size_t);
    void do_receive_payload(const size_t length);
    void when_payload_received(const size_t, const std::error_code &,
                               const size_t);
    void do_send_message(message::ZeroCopyBuffer &&msg);
    void when_message_sent(const size_t, const std::error_code &, const size_t);
    void do_send_header(message::ZeroCopyBuffer &&header);
    std::string socket_to_string() const;

    virtual void when_timeout(const std::error_code &ec) = 0;
    virtual void header_parser() = 0;
    virtual void handle_eof() = 0;
};

class ServerConnection : public Connection {
  public:
    ServerConnection(execution_context &_context, asio_tcp::socket &&s,
                     const receive_callback &f);
    void when_receive_heartbeat();
    void when_timeout(const std::error_code &ec) override;
    void header_parser() override;
    void handle_eof() override;
};

class ClientConnection : public Connection {
  public:
    ClientConnection(execution_context &_context, asio_tcp::socket &&s,
                     const receive_callback &f);
    void do_send_heartbeat();
    void when_timeout(const std::error_code &ec) override;
    void header_parser() override;
    void handle_eof() override;
};

_THALLIUM_END_NAMESPACE

#endif
