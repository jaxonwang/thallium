#include "network.hpp" 
#include "common.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

class BasicModel: public Layers{

    public:
    Layers * lower;
    void send(const int conn_id, message::ZeroCopyBuffer &&msg);

    void receive(const int conn_id, const char * buf, const size_t length);

    virtual void logic(int conn_id, message::CopyableBuffer &msg) = 0;

    virtual void start(){}
};

_THALLIUM_END_NAMESPACE
