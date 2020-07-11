#include "heartbeat.hpp"

_THALLIUM_BEGIN_NAMESPACE

const std::chrono::seconds HeartBeatPolicy::interval{
    TI_HEARTBEAT_INTERVAL_SECONDS};
const std::chrono::seconds HeartBeatPolicy::timeout{
    2 * TI_HEARTBEAT_INTERVAL_SECONDS};
const std::chrono::seconds HeartBeatPolicy::server_initial_timeout{
    HeartBeatPolicy::interval + HeartBeatPolicy::timeout};

_THALLIUM_END_NAMESPACE
