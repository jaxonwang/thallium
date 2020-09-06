#ifndef _THALLIUM_NETWORK_CONNECTION
#define _THALLIUM_NETWORK_CONNECTION

#include <boost/asio.hpp>
#include <functional>
#include <queue>
#include <unordered_map>

#include "asio_type_wrapper.hpp"
#include "behavior.hpp"
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
    typedef std::function<void(const message::ConnectionEvent)> event_callback;

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
    event_callback upper_layer_event_callback;
    void do_write();

  public:
    Connection(execution_context &_context, asio_tcp::socket &&s,
               const receive_callback &f, const event_callback &e_c);
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
                     const receive_callback &f, const event_callback &ec);

  protected:
    void header_parser() override;
    void handle_eof() override;
};

class ClientConnection : public Connection {
  public:
    ClientConnection(execution_context &_context, asio_tcp::socket &&s,
                     const receive_callback &f, const event_callback &ec);

  protected:
    void header_parser() override;
    void handle_eof() override;
};

class ServerConnectionWithHeartbeat : public ServerConnection,
                                      private HeartbeatChecker {
  public:
    ServerConnectionWithHeartbeat(execution_context &_context,
                                  asio_tcp::socket &&s,
                                  const receive_callback &f,
                                  const event_callback &ec);

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
                                  const receive_callback &f,
                                  const event_callback &ec);

  public:
    void connection_close() override;

  protected:
    void send_heartbeat(const std::error_code &ec) override;
};

class ConnectionManager : public Layer, public Disconnector {
  public:
    virtual ~ConnectionManager() = default;
    // when new connection accepted, call this function
    virtual void new_connection(boost::asio::ip::tcp::socket &&s) = 0;

    virtual void send(const int, message::ZeroCopyBuffer &&) = 0;
    virtual void disconnect(const int) = 0;

    virtual void receive(const int, const char *, const size_t) = 0;
    virtual void event(const int, const message::ConnectionEvent &e) = 0;
    virtual void set_upper(Layer *upper) = 0;
};

template <class ConnectionType>
class RealConnectionManager : public ConnectionManager {
  private:
    execution_context &_context;
    std::unordered_map<int, std::unique_ptr<ConnectionType>> holdings;
    int next_id;
    Layer *_upper;

  public:
    RealConnectionManager(execution_context &_context)
    : _context(_context), holdings(), next_id(0) {
        // TODO: periodically clean up closed connections
    }
    RealConnectionManager(const ConnectionManager &c) = delete;

    // when new connection accepted, call this function
    void new_connection(boost::asio::ip::tcp::socket &&peer) override {
        // thread unsafe won't be called by multi threads
        int conn_id = next_id++;
        typedef Connection::receive_callback receive_callback;
        typedef Connection::event_callback event_callback;
        // add callback
        receive_callback f = std::bind(&ConnectionManager::receive, this, conn_id,
                                  std::placeholders::_1, std::placeholders::_2);
        event_callback e_f = std::bind(&ConnectionManager::event, this, conn_id,
                                  std::placeholders::_1);
        auto new_con_ptr =
            new ConnectionType(_context, std::move(peer), f, e_f);
        holdings[conn_id] = std::unique_ptr<ConnectionType>{new_con_ptr};
        holdings[conn_id]->do_receive_message();
    }

    void send(const int conn_id, message::ZeroCopyBuffer &&msg) override {
        holdings[conn_id]->do_send_message(move(msg));
    }
    void disconnect(const int conn_id) override {
        holdings[conn_id]->connection_close();
    }

    void receive(const int conn_id, const char *buf,
                 const size_t length) override {
        _upper->receive(conn_id, buf, length);
    }
    void event(const int conn_id, const message::ConnectionEvent &e) override {
        _upper->event(conn_id, e);
    }

    void set_upper(Layer *upper) override { _upper = upper; }
};

_THALLIUM_END_NAMESPACE

#endif
