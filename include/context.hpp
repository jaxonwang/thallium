#ifndef _THALLIUM_CONTEXT_HEADER
#define _THALLIUM_CONTEXT_HEADER

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

extern const char * ENVNAME_COORD_HOST;
extern const char * ENVNAME_COORD_PORT;
extern const char * ENVNAME_STARTUP_COOKIE;

class Context{
public:
    Context(){};
    const char *coordinator_host;
    int coordinator_port;
};


_THALLIUM_END_NAMESPACE


#endif
