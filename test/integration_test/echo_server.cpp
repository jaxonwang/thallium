
#include <iostream>
#include <thread>

#include "logging.hpp"
#include "network/asio_type_wrapper.hpp"
#include "network/behavior.hpp"
#include "network/client.hpp"
#include "network/network.hpp"
#include "network/protocol.hpp"
#include "network/server.hpp"

#define BOOST_ASIO_ENABLE_HANDLER_TRACKING

static int port;

using namespace std;
using namespace thallium;

class ServerImpl : public ServerModel {
    void logic(int conn_id, const char* buf, const size_t length) override {
        message::CopyableBuffer msg(buf, buf + length);

        send(conn_id, message::ZeroCopyBuffer(move(msg)));
        stop();
    }
};

int main() {
    logging_init(0);
    execution_context ctx{1};

    std::error_code ec;
    ti_socket_t skt = {33333, resolve("127.0.0.1", ec)};
    AsyncServer s{ctx, skt};

    ServerImpl s_impl;
    RunServer(s_impl, s);

    port = s.server_socket().port;

    ctx.run();
}
