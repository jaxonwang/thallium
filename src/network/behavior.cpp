

#include "behavior.hpp"

_THALLIUM_BEGIN_NAMESPACE

#include <string>

void BasicModel::send(const int conn_id, message::ZeroCopyBuffer &&msg) {
    lower->send(conn_id, move(msg));
}

void BasicModel::receive(const int conn_id, const char *buf,
                         const size_t length) {
    // TODO: a copy here
    message::CopyableBuffer msg{buf, buf + length};
    logic(conn_id, msg);
}

void ClientModel::start(){
    init_logic();
}

_THALLIUM_END_NAMESPACE
