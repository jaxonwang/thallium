#include "heartbeat.hpp" 

_THALLIUM_BEGIN_NAMESPACE

const std::chrono::seconds HeartBeatPolicy::interval{TI_HEARTBEAT_INTERVAL_SECONDS};
const std::chrono::seconds HeartBeatPolicy::timeout{2 * TI_HEARTBEAT_INTERVAL_SECONDS};

_THALLIUM_END_NAMESPACE
