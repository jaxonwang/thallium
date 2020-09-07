
#include "heartbeat.hpp"
#include "logging.hpp"

#include <functional>

_THALLIUM_BEGIN_NAMESPACE

using namespace std::placeholders;

#ifndef TI_HEARTBEAT_INTERVAL_SECONDS
    #define TI_HEARTBEAT_INTERVAL_SECONDS 2
#endif

std::chrono::milliseconds HeartBeatPolicy::interval{
    TI_HEARTBEAT_INTERVAL_SECONDS};
std::chrono::milliseconds HeartBeatPolicy::timeout{
    2 * TI_HEARTBEAT_INTERVAL_SECONDS};
std::chrono::milliseconds HeartBeatPolicy::server_initial_timeout{
    HeartBeatPolicy::interval + HeartBeatPolicy::timeout};

HeartbeatChecker::HeartbeatChecker(execution_context &_context)
    : interval(HeartBeatPolicy::timeout),
      timer(_context, HeartBeatPolicy::server_initial_timeout) {}

void HeartbeatChecker::heartbeat_start() {
    timer.async_wait(std::bind(&HeartbeatChecker::when_timeout, this, _1));
}

using namespace std;

void HeartbeatChecker::heartbeat_received() {
    // auto remain0 = chrono::duration_cast<chrono::nanoseconds>(timer.expires_from_now());
    timer.expires_from_now(interval);
    // auto remain1 = chrono::duration_cast<chrono::nanoseconds>(timer.expires_from_now());
    // cout << remain0.count() << " " << remain1.count() << endl;
    timer.async_wait(std::bind(&HeartbeatChecker::when_timeout, this, _1));
}

void HeartbeatChecker::heartbeat_stop() { 
    timer.cancel(); 
    // TI_DEBUG("The heartbeat timer is cancelled.");
}

HeartbeatSender::HeartbeatSender(execution_context &_context)
    : interval(HeartBeatPolicy::interval),
      timer(_context, HeartBeatPolicy::interval) {}

void HeartbeatSender::heartbeat_start() {
    timer.async_wait(std::bind(&HeartbeatSender::send_and_reset, this, _1));
}

void HeartbeatSender::send_and_reset(const std::error_code &ec) {
    // propagate error handling to implementation
    send_heartbeat(ec);
    if (!ec) {
        timer.expires_from_now(interval);
        timer.async_wait(std::bind(&HeartbeatSender::send_and_reset, this, _1));
    }
}

void HeartbeatSender::heartbeat_stop() { timer.cancel(); }

_THALLIUM_END_NAMESPACE
