#ifndef _THALLIUM_NETWORK_HEARTBEAT
#define _THALLIUM_NETWORK_HEARTBEAT

#include <chrono>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

struct HeartBeatPolicy {
    static const std::chrono::seconds interval;
    static const std::chrono::seconds timeout;
    // initial timeout is the timemout for the first heartbeat
    static const std::chrono::seconds server_initial_timeout;
};

constexpr static int TI_HEARTBEAT_INTERVAL_SECONDS = 5;

_THALLIUM_END_NAMESPACE

#endif
