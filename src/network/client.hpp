#ifndef _THALLIUM_NETWORK_CLIENT
#define _THALLIUM_NETWORK_CLIENT

#include "common.hpp"
#include "io_loop.hpp"
#include "network.hpp"
#include "behavior.hpp"
#include "protocol.hpp"
#include "connection.hpp"

_THALLIUM_BEGIN_NAMESPACE

class AsyncClient : public Layer, public Disconnector, public Stoper{
  public:
    AsyncClient(execution_context &ctx, const ti_socket_t &);
    AsyncClient(execution_context &ctx, const std::string &, unsigned short port);
    void start();
    void stop() override;
    ti_socket_t client_socket();
    Layer *upper;

  private:
    ti_socket_t _socket;
    execution_context &_context;  // mystrious context used by asio
    std::unique_ptr<ClientConnection> holding;
    void receive(const int, const char *, const size_t) override;
    void send(const int, message::ZeroCopyBuffer &&msg) override;
    void disconnect(const int) override;
    template<class Endpoints>
    real_addr_type try_connect(boost::asio::ip::tcp::socket & asio_s, Endpoints & eps);
};

void RunClient(ClientModel &, AsyncClient &);

_THALLIUM_END_NAMESPACE

#endif
