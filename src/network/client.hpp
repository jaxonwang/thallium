#ifndef _THALLIUM_NETWORK_CLIENT
#define _THALLIUM_NETWORK_CLIENT

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
    void receive(const int, const char *, const size_t) override;
    void send(const int, message::ZeroCopyBuffer &&msg) override;
    void disconnect(const int) override;
    template <class Endpoints>
    real_addr_type try_connect(boost::asio::ip::tcp::socket &asio_s,
                               Endpoints &eps);
};

void RunClient(ClientModel &, AsyncClient &);

_THALLIUM_END_NAMESPACE

#endif
