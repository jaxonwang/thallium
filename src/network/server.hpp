#ifndef _THALLIUM_NETWORK_SERVER
#define _THALLIUM_NETWORK_SERVER

#include <boost/asio.hpp>
#include <unordered_map>

#include "common.hpp"
#include "behavior.hpp"
#include "protocol.hpp"
#include "connection.hpp"

_THALLIUM_BEGIN_NAMESPACE

class AsyncServer : public Stoper{
  private:
    ti_socket_t _socket;
    execution_context &_context;  // mystrious context used by asio
    boost::asio::ip::tcp::acceptor _acceptor;

    void when_accept(const std::error_code &ec, boost::asio::ip::tcp::socket &&peer);
    void do_accept();
  public:
    AsyncServer(execution_context &ctx, const ti_socket_t &, const bool heartbeat=false);
    void start();
    void stop() override;
    ti_socket_t server_socket();

    std::unique_ptr<ConnectionManager> _cmanager;

};

void RunServer(ServerModel &s_impl, AsyncServer &s_async);

_THALLIUM_END_NAMESPACE
#endif
