#include "test.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

#ifdef __linux__
#include <cstdlib>
#endif

#include <unistd.h>

#include "logging.hpp"

namespace ti_test {

TmpFile::TmpFile()
    : filepath_holder{"/tmp/justatempfilenameXXXXXX"},
      filepath(filepath_holder) {
    int ret = mkstemp(filepath_holder);

    if (!ret) throw std::runtime_error("Can not find a tmp filename");

    close(ret);
}

TmpFile::~TmpFile() { std::remove(filepath); }

struct LoggingTracerImpl {
    std::unique_ptr<thallium::GlobalLoggerManager> mnger_holder;
    std::unique_ptr<TmpFile> logfile;
    bool changelevelonly;
    bool has_swap_back;
};

// class LoggingTracer{
//     public:
//     explicit LoggingTracer(const int level);
//     ~LoggingTracer();
//     std::vector<std::string> log_content();
// };

LoggingTracer::LoggingTracer(const int level, const bool changelevelonly)
    : impl(new LoggingTracerImpl()) {
    impl->changelevelonly = changelevelonly;
    // has_swap_back = false;
    // store current
    thallium::get_global_manager().swap(impl->mnger_holder);
    if (!changelevelonly) {  // no trace the log
        impl->logfile.reset(new TmpFile());
        thallium::logging_init(level, impl->logfile->filepath);
    } else {
        thallium::logging_init(level);
    }
}

LoggingTracer::~LoggingTracer() {
    // revert to before
    thallium::get_global_manager().swap(impl->mnger_holder);
}

std::vector<std::string> LoggingTracer::collect() {
    std::vector<std::string> content;
    if (impl->changelevelonly) return content;
    thallium::get_global_manager()->flush_records();

    std::ifstream reader{impl->logfile->filepath};
    std::string line;
    while (std::getline(reader, line)) {
        content.push_back(move(line));
    }
    return content;
}

}  // namespace ti_test
