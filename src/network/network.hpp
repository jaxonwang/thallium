#ifndef _THALLIUM_NETWORK_HEADER
#define _THALLIUM_NETWORK_HEADER

#include <memory>
#include <string>

#include "asio_type_wrapper.hpp"
#include "common.hpp"
#include "protocol.hpp"
#include "behavior.hpp"
#include "client.hpp"
#include "server.hpp"

_THALLIUM_BEGIN_NAMESPACE

// in network utils

bool valid_hostname(const char *);

bool valid_port(const int);

int port_to_int(const char *);


_THALLIUM_END_NAMESPACE
#endif
