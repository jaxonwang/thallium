#ifndef _THALLIUM_PLACE_HEADER
#define _THALLIUM_PLACE_HEADER

#include <memory>
#include <unordered_map>

#include "common.hpp"
#include "utils.hpp"
#include "network/network.hpp"

_THALLIUM_BEGIN_NAMESPACE

typedef int rank_type;

class Place {  // handle
  public:
    rank_type place_rank;
};

class PlaceObj {  // real place object
  public:
    rank_type rank;
    bool is_local;  //
    ti_socket_t listen_socks;
    PlaceObj():is_local(false){}
    PlaceObj(rank_type rank):rank(rank),is_local(false){}
};

_THALLIUM_END_NAMESPACE

#endif
