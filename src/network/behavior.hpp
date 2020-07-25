#ifndef _THALLIUM_NETWORK_BEHAVIOR
#define _THALLIUM_NETWORK_BEHAVIOR

#include "common.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

class Layer {
  public:
    virtual void send(const int conn_id, message::ZeroCopyBuffer &&msg) = 0;
    virtual void receive(const int conn_id, const char *, const size_t) = 0;
};

class Disconnector {
  public:
    virtual void disconnect(const int conn_id) = 0;
};

class Stoper {
  public:
    virtual void stop() = 0;
};

class BasicModel : public Layer {
  public:
    Layer *lower;
    Disconnector *disconnector;
    Stoper *stopper;

    // non virtual is to be used by subclass
    void send(const int conn_id, message::ZeroCopyBuffer &&msg);

    void receive(const int conn_id, const char *buf, const size_t length);

    void disconnect(const int conn_id);

    void stop();

    virtual void logic(int conn_id, const message::ReadOnlyBuffer &) = 0;

    virtual void start(){};
};

class ServerModel : public BasicModel {};

class ClientModel : public BasicModel {
    virtual void init_logic() = 0;

  public:
    void start() override;
};

_THALLIUM_END_NAMESPACE

#endif
