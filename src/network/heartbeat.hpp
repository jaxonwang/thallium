#ifndef _THALLIUM_HEARTBEAT
#define _THALLIUM_HEARTBEAT

#include <chrono>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

struct HeartBeatPolicy {
    static const std::chrono::seconds interval;
    static const std::chrono::seconds timeout;
};

constexpr static int TI_HEARTBEAT_INTERVAL_SECONDS = 5;

_THALLIUM_END_NAMESPACE

#endif
