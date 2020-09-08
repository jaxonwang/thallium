#ifndef _THALLIUM_NETWORK_CLIENT
#define _THALLIUM_NETWORK_CLIENT

#include <unordered_map>
#include <unordered_set>

#include "behavior.hpp"
#include "common.hpp"
#include "connection.hpp"
#include "io_loop.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

class AsyncClient : public Layer, public Disconnector, public Stoper {
  public:
    AsyncClient(execution_context &ctx, const ti_socket_t &,
                const bool heartbeat = false);
    AsyncClient(execution_context &ctx, const std::string &,
                const unsigned short port, const bool heartbeat = false);
    void start();
    void stop() override;
    ti_socket_t client_socket();
    Layer *upper;

  private:
    ti_socket_t _socket;
    execution_context &_context;  // mystrious context used by asio
    std::unique_ptr<ClientConnection> holding;
    bool withheartbeat;
    void send(const int, message::ZeroCopyBuffer &&msg) override;
    void disconnect(const int) override;

    void receive(const int, const char *, const size_t) override;
    void event(const int, const message::ConnectionEvent &e) override;

    void init_connection(boost::asio::ip::tcp::socket &&);
};

class MultiClient : public Layer, public Disconnector, public Stoper {
  private:
    execution_context &_context;

  public:
    typedef std::unordered_map<int, std::pair<std::string, int>> address_book_t;

  private:
    address_book_t address_book;  // id-> addr,port
    RealConnectionManager<ClientConnection> conmanager;
    std::unordered_set<int> established;

    void do_connect(const int);

  public:
    MultiClient(execution_context &ctx, const address_book_t &address_book);
    void start();
    void stop() override;
    Layer *upper;

  private:
    void send(const int, message::ZeroCopyBuffer &&msg) override;
    void disconnect(const int) override;

    void receive(const int, const char *, const size_t) override;
    void event(const int, const message::ConnectionEvent &e) override;
};

template <class ClientModelType, class ClientType>
void RunClient(ClientModelType &c_impl, ClientType &c_async) {
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

#endif
