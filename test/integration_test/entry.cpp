#include <exception>

#define THALLIUM_ENABLE_BACKTRACE
#include "thallium.hpp"
#include "exception.hpp"


void run2(){
    auto e = std::runtime_error("please backtrace me!");
    TI_RAISE(e);
}
void run1(){
    run2();
}
int ti_main(int argc, const char *argv[]) {
    run1();
    return 0;
}

TI_MAIN()

