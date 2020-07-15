#ifndef _THALLIUM_NETWORK_SERVER
#define _THALLIUM_NETWORK_SERVER

#include <boost/asio.hpp>
#include <unordered_map>

#include "common.hpp"
#include "io_loop.hpp"
#include "network.hpp"
#include "behavior.hpp"
#include "protocol.hpp"
#include "connection.hpp"

_THALLIUM_BEGIN_NAMESPACE

class ConnectionManager : public Layer, public Disconnector{

  private:
    execution_context & _context;
    std::unordered_map<int, std::unique_ptr<ServerConnection>> holdings;
    int next_id;

  public:
    ConnectionManager(execution_context &_context);
    ConnectionManager(const ConnectionManager &c) = delete;

    void new_connection(boost::asio::ip::tcp::socket &&s);

    void inline send(const int, message::ZeroCopyBuffer &&) override;
    void inline receive(const int, const char *, const size_t) override;
    void inline disconnect(const int) override;

    Layer *upper;
};

class AsyncServer : public Stoper{
  private:
    ti_socket_t _socket;
    execution_context &_context;  // mystrious context used by asio
    boost::asio::ip::tcp::acceptor _acceptor;

    void when_accept(const std::error_code &ec, boost::asio::ip::tcp::socket &&peer);
    void do_accept();
  public:
    AsyncServer(execution_context &ctx, const ti_socket_t &);
    void start();
    void stop() override;
    ti_socket_t server_socket();

    ConnectionManager _cmanager;

};

void RunServer(ServerModel &s_impl, AsyncServer &s_async);

_THALLIUM_END_NAMESPACE
#endif
