
#include <iostream>
#include <thread>

#include "logging.hpp" 
#include "network/protocol.hpp"
#include "network/network.hpp"
#include "network/server.hpp"
#include "network/client.hpp"
#include "network/behavior.hpp"
#include "network/asio_type_wrapper.hpp"

static int port;

using namespace std;
using namespace thallium;


class ServerImpl: public ServerModel{

    void logic(int conn_id, message::CopyableBuffer &msg) override{

        send(conn_id, message::ZeroCopyBuffer(move(msg)));
    }

};

class ClientImpl:public ClientModel{

    void logic(int conn_id, message::CopyableBuffer &msg) override{
        cout << msg.data() << endl;
        disconnect(conn_id);
        stop();
    }

    void init_logic() override{
        string s = "sdafsdagfasl;dfka";
        message::CopyableBuffer b{s.begin(), s.end()};
        send(0, message::ZeroCopyBuffer(move(b)));
    }
};

void server(execution_context &ctx){

    std::error_code ec;
    ti_socket_t skt = {0, resolve("127.0.0.1", ec)};
    AsyncServer s{ctx, skt};

    ServerImpl s_impl;
    RunServer(s_impl, s);

    port = s.server_socket().port;

}

void client(){
    execution_context ctx{1};

    AsyncClient c(ctx, "127.0.0.1", port);

    ClientImpl c_impl;
    RunClient(c_impl, c);

    ctx.run();

}

int main()
{
    logging_init(0);
    execution_context ctx{1};
    
    server(ctx);
    auto t2 = thread(client);

    ctx.run();
    t2.join();
}


