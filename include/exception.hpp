#ifndef _THALLIUM_EXCEPTION_HPP
#define _THALLIUM_EXCEPTION_HPP

#include <exception>

#ifdef THALLIUM_ENABLE_BACKTRACE
#include <boost/exception/all.hpp>
#include <boost/stacktrace.hpp>

typedef boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>
    traced;
typedef boost::error_info<struct tag_file_line, std::string> file_line;
#endif

namespace thallium {

namespace ti_exception {
// class ExceptionBase : boost::exception {};
//

#ifdef THALLIUM_ENABLE_BACKTRACE
#define TI_RAISE(x)            {                                        \
    std::string f_l =                                                  \
        std::string{__FILE__} + ", line: " + std::to_string(__LINE__); \
    throw thallium::ti_exception::raise(x) << file_line(f_l);}
#else
#define TI_RAISE(x) { throw x; }
#endif

template <typename E>
#ifdef THALLIUM_ENABLE_BACKTRACE
auto raise(const E &e) -> decltype(boost::enable_error_info(e)) {
    return boost::enable_error_info(e)
           << traced(boost::stacktrace::stacktrace());
}
#endif

inline void handle_uncatched_and_abort(const std::exception &e) {
#ifdef THALLIUM_ENABLE_BACKTRACE
    std::cerr << "Exception: " << e.what() << std::endl;
    std::cerr << "in: "<< *boost::get_error_info<file_line>(e) << std::endl;
    const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(e);
    if (st) {
        std::cerr << "stacktrace:" << std::endl << *st << std::endl;
    }
    std::abort();
#else
    throw e;
#endif
}

}  // namespace ti_exception

}  // namespace thallium
#endif
