#include "io_loop.hpp" 

#include <csignal>


_THALLIUM_BEGIN_NAMESPACE

std::unique_ptr<execution_context> &&init_io_main_loop(){ 

    std::unique_ptr<execution_context>main_ctx{new execution_context{1}} ;

    thallium::signal_set _signals(*main_ctx);

#if defined(SIGPIPE)
    _signals.add(SIGPIPE);
#endif
    // just ignore signals
    _signals.async_wait([](const std::error_code &, int) {});

    return std::move(main_ctx);
}

void main_loop(execution_context &ctx){
    ctx.run();
}

_THALLIUM_END_NAMESPACE
