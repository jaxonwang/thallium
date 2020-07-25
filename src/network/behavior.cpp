#include "behavior.hpp"

_THALLIUM_BEGIN_NAMESPACE

#include <string>

void BasicModel::send(const int conn_id, message::ZeroCopyBuffer &&msg) {
    lower->send(conn_id, move(msg));
}

void BasicModel::receive(const int conn_id, const char *buf,
                         const size_t length) {
    logic(conn_id, buf, length);
}

void BasicModel::disconnect(const int conn_id){
    disconnector->disconnect(conn_id);
}

void BasicModel::stop(){
    stopper->stop();
}

void ClientModel::start(){
    init_logic();
}


_THALLIUM_END_NAMESPACE
