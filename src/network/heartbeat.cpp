
#include <functional>
#include "heartbeat.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std::placeholders;

const std::chrono::seconds HeartBeatPolicy::interval{
    TI_HEARTBEAT_INTERVAL_SECONDS};
const std::chrono::seconds HeartBeatPolicy::timeout{
    2 * TI_HEARTBEAT_INTERVAL_SECONDS};
const std::chrono::seconds HeartBeatPolicy::server_initial_timeout{
    HeartBeatPolicy::interval + HeartBeatPolicy::timeout};

HeartbeatChecker::HeartbeatChecker(execution_context &_context,
                 const std::chrono::milliseconds &interval)
    : interval(interval), timer(_context, interval) {}

void HeartbeatChecker::heartbeat_start(){
  timer.async_wait(std::bind(&HeartbeatChecker::when_timeout, this, _1));
}

void HeartbeatChecker::heartbeat_received(){
    timer.expires_from_now(HeartBeatPolicy::timeout);
}

void HeartbeatChecker::heartbeat_stop(){
    timer.cancel();
}

HeartbeatSender::HeartbeatSender(execution_context &_context,
                 const std::chrono::milliseconds &interval)
    : timer(_context, interval) {}

void HeartbeatSender::heartbeat_start(){
  timer.async_wait(std::bind(&HeartbeatSender::send_heartbeat, this, _1));
}

void HeartbeatSender::heartbeat_stop(){
    timer.cancel();
}

_THALLIUM_END_NAMESPACE
