#ifndef _THALLIUM_NETWORK_CONNECTION
#define _THALLIUM_NETWORK_CONNECTION

#include <boost/asio.hpp>
#include <functional>
#include <queue>

#include "asio_type_wrapper.hpp"
#include "common.hpp"
#include "heartbeat.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

class Connection {
  public:
    // use char * to show that the the data in char * will be availabe
    // during the whole callback execution, with the guarantee that connection
    // is singal thread read
    typedef std::function<void(const char *, size_t)> receive_callback;

  protected:
    using asio_tcp = boost::asio::ip::tcp;
    typedef std::unique_ptr<asio_tcp::socket> socket_ptr;
    typedef std::vector<message::ZeroCopyBuffer> buffer_sequence;

    const static size_t buffer_size = 8192;
    execution_context &_context;
    socket_ptr holding_socket;
    // this buffer will only grow but never shinks
    std::vector<char> _receive_buffer;
    std::array<char, message::header_size> _header_buffer;
    std::queue<std::unique_ptr<buffer_sequence>> _write_queue;
    std::string socket_str;

    receive_callback upper_layer_callback;
    void do_write();

  public:
    Connection(execution_context &_context, asio_tcp::socket &&s,
               const receive_callback &f);
    Connection(const Connection &c) = delete;
    virtual ~Connection() = default;

    virtual void connection_close();
    virtual void do_send_message(message::ZeroCopyBuffer &&msg);
    virtual void do_receive_message();

  protected:
    void when_header_received(const std::error_code &, const size_t);
    void do_receive_payload(const size_t length);
    void when_payload_received(const size_t, const std::error_code &,
                               const size_t);
    void when_message_sent(const size_t, const std::error_code &, const size_t);
    void do_send_header(message::ZeroCopyBuffer &&header);
    std::string socket_to_string() const;

    virtual void header_parser() = 0;
    virtual void handle_eof() = 0;
};

class ServerConnection : public Connection {
  public:
    ServerConnection(execution_context &_context, asio_tcp::socket &&s,
                     const receive_callback &f);

  protected:
    void header_parser() override;
    void handle_eof() override;
};

class ClientConnection : public Connection {
  public:
    ClientConnection(execution_context &_context, asio_tcp::socket &&s,
                     const receive_callback &f);

  protected:
    void header_parser() override;
    void handle_eof() override;
};

class ServerConnectionWithHeartbeat : public ServerConnection,
                                      private HeartbeatChecker {
  public:
    ServerConnectionWithHeartbeat(execution_context &_context,
                                  asio_tcp::socket &&s,
                                  const receive_callback &f);

  public:
    void connection_close() override;

  protected:
    void when_receive_heartbeat();
    void header_parser() override;
    void when_timeout(const std::error_code &ec) override;
};

class ClientConnectionWithHeartbeat : public ClientConnection,
                                      private HeartbeatSender {
  public:
    ClientConnectionWithHeartbeat(execution_context &_context,
                                  asio_tcp::socket &&s,
                                  const receive_callback &f);

  public:
    void connection_close() override;

  protected:
    void send_heartbeat(const std::error_code &ec) override;
};

_THALLIUM_END_NAMESPACE

#endif
