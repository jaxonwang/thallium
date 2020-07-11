#ifndef _THALLIUM_NETWORK_IO_LOOPS
#define _THALLIUM_NETWORK_IO_LOOPS

#include <memory>

#include "common.hpp"
#include "asio_type_wrapper.hpp"

_THALLIUM_BEGIN_NAMESPACE

std::unique_ptr<execution_context> &&init_io_main_loop(); 

void main_loop(execution_context &ctx);

_THALLIUM_END_NAMESPACE
#endif
