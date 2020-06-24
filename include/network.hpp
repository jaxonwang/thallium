#ifndef _THALLIUM_NETWORK_HEADER
#define _THALLIUM_NETWORK_HEADER

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

bool valid_hostname(const char *);

bool valid_port(const int);

int port_to_int(const char *);

_THALLIUM_END_NAMESPACE
#endif
