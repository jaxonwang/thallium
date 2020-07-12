#ifndef _THALLIUM_NETWORK_CLIENT
#define _THALLIUM_NETWORK_CLIENT

#include "common.hpp"
#include "io_loop.hpp"
#include "network.hpp"
#include "behavior.hpp"
#include "protocol.hpp"
#include "connection.hpp"

_THALLIUM_BEGIN_NAMESPACE

class AsyncClient : virtual public Client, virtual public Layers {
  public:
    AsyncClient(execution_context &ctx, const ti_socket_t &);
    void start() override;
    void stop() override;
    ti_socket_t client_socket() override;
    Layers *upper;

  private:
    ti_socket_t _socket;
    execution_context &_context;  // mystrious context used by asio
    std::unique_ptr<ClientConnection> holding;
    void receive(const int, const char *, const size_t) override;
    void send(const int, message::ZeroCopyBuffer &&msg) override;
};

template<class ClientLogic>
void RunClient(const ti_socket_t & s){
    static_assert(std::is_base_of<ClientModel, ClientLogic>::value, "ClientLogic should be ClientModel!");
    auto && ctx_ptr = init_io_main_loop();

    ClientLogic c_impl;
    AsyncClient c_async{*ctx_ptr, s};
    c_impl.lower = c_async;
    c_async.upper = c_impl;
    c_async.start();
    c_impl.start();

    main_loop(*ctx_ptr);
}

_THALLIUM_END_NAMESPACE

#endif
