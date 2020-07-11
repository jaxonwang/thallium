#include <boost/asio.hpp>

#include "common.hpp"
#include "logging.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "heartbeat.hpp"

_THALLIUM_BEGIN_NAMESPACE


class AsyncClient: public Client{
  public:
    AsyncClient(execution_context & ctx, const ti_socket_t &);
    void start() override;
    void stop() override;
    ti_socket_t client_socket() override;
  private:
    ti_socket_t _socket;
    execution_context &_context;  // mystrious context used by asio
};


_THALLIUM_END_NAMESPACE
