#ifndef _THALLIUM_NETWORK_BEHAVIOR
#define _THALLIUM_NETWORK_BEHAVIOR

#include "common.hpp"
#include "protocol.hpp"

_THALLIUM_BEGIN_NAMESPACE

class Layer {
  public:
    virtual void send(const int conn_id, message::ZeroCopyBuffer &&msg) = 0;
    virtual void receive(const int conn_id, const char *, const size_t) = 0;
    virtual void event(const int conn_id, const message::ConnectionEvent &e) = 0;
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
    // active call
    void send(const int conn_id, message::ZeroCopyBuffer &&msg);
    void disconnect(const int conn_id);
    // methods been called
    void receive(const int conn_id, const char *buf, const size_t length);
    // default impl is ignore all event
    void event(const int, const message::ConnectionEvent &){}

    // neither server or client do close() in this operation
    void stop();

    virtual void logic(const int conn_id, const message::ReadOnlyBuffer &) = 0;

    virtual void start(){};
};

class ServerModel : public BasicModel {};

class ClientModel : public BasicModel {
  protected:
    virtual void init_logic() = 0;

  public:
    // a method to avoid using explicit conn_id
    void send_to_server(message::ZeroCopyBuffer &&msg);
    void start() override;
};

template <class RealModel, class AbstractModel>
class StateMachine: public AbstractModel {
    RealModel &m_obj;
    void (RealModel::*current_state)(const int, const message::ReadOnlyBuffer &);
    typedef void (RealModel::*state_handler_t)(const int,
                                           const message::ReadOnlyBuffer &);

  public:
    StateMachine(RealModel &m_obj, const state_handler_t start_state)
        : m_obj(m_obj), current_state(start_state) {}
    StateMachine(const StateMachine &) = delete;
    StateMachine(StateMachine &&) = delete;
    void run_state(const int conn_id, const message::ReadOnlyBuffer &buf) {
        (m_obj.*current_state)(conn_id, buf);
    }
    void go_to_state(const state_handler_t next_state) {
        current_state = next_state;
    }
    void error_state(const std::string &errmsg) {
        throw std::runtime_error(errmsg);
    }

    void logic(const int conn_id, const message::ReadOnlyBuffer &buf) override {
        run_state(conn_id, buf);
    }
};

_THALLIUM_END_NAMESPACE

#endif
