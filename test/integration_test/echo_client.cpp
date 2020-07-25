
#include <iostream>
#include <thread>

#include "logging.hpp" 
#include "network/protocol.hpp"
#include "network/network.hpp"
#include "network/server.hpp"
#include "network/client.hpp"
#include "network/behavior.hpp"
#include "network/asio_type_wrapper.hpp"


#define BOOST_ASIO_ENABLE_HANDLER_TRACKING

using namespace std;
using namespace thallium;

static const string s = "sdafsdagfasl;dfka\n";

class ClientImpl:public ClientModel{

    void logic(int conn_id, const char* buf, const size_t length) override {
        string s1(buf, length);
        if(!(s == s1))
            cerr << "The Echoed is not the same!" << endl;
        disconnect(conn_id);
        stop();
    }

    void init_logic() override{
        message::CopyableBuffer b{s.begin(), s.end()};
        send(0, message::ZeroCopyBuffer(move(b)));
    }
};


void client(){
    execution_context ctx{1};

    AsyncClient c(ctx, "127.0.0.1", 33333);

    ClientImpl c_impl;
    RunClient(c_impl, c);

    ctx.run();
}

int main()
{
    logging_init(0);
    client();
}


