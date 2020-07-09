#ifndef _THALLIUM_ASIO_WRAPPER
#define _THALLIUM_ASIO_WRAPPER

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio.hpp>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

typedef boost::asio::ip::address_v4 real_addr_type; 

typedef boost::asio::io_context execution_context; 

typedef boost::asio::signal_set signal_set; 

real_addr_type hostname(const char *);

_THALLIUM_END_NAMESPACE


#endif
