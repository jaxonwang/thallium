#ifndef _THALLIUM_NETWORK_HEARTBEAT
#define _THALLIUM_NETWORK_HEARTBEAT

#include <chrono>

#include "common.hpp"
#include "asio_type_wrapper.hpp"

_THALLIUM_BEGIN_NAMESPACE

struct HeartBeatPolicy {
    static const std::chrono::seconds interval;
    static const std::chrono::seconds timeout;
    // initial timeout is the timemout for the first heartbeat
    static const std::chrono::seconds server_initial_timeout;
};

class HeartbeatChecker {
    const std::chrono::milliseconds interval;
    boost::asio::steady_timer timer;

  public:
    explicit HeartbeatChecker(execution_context & _context, const std::chrono::milliseconds &interval);
    void heartbeat_start();
    void heartbeat_received();
    virtual void when_timeout(const std::error_code &ec) = 0;
    void heartbeat_stop();
};

class HeartbeatSender{
    boost::asio::steady_timer timer;

  public:
    explicit HeartbeatSender(execution_context & _context, const std::chrono::milliseconds &timeout);
    void heartbeat_start();
    virtual void send_heartbeat(const std::error_code &ec) = 0;
    void heartbeat_stop();
};

constexpr static int TI_HEARTBEAT_INTERVAL_SECONDS = 5;

_THALLIUM_END_NAMESPACE

#endif
