#ifndef _THALLIUM_TEST
#define _THALLIUM_TEST

#include <cstdio>
#include <memory>

#include "gtest/gtest.h"

#define ASSERT_THROW_WITH(statement, msg)                        \
    do {                                                         \
        try {                                                    \
            statement;                                           \
            ASSERT_TRUE(false) << "The statement should throw!"; \
        } catch (std::exception & e) {                           \
            ASSERT_STREQ(e.what(), msg);                         \
        }                                                        \
    } while (0)

#define ASSERT_THROW_WITH_TYPE(statement, exception_t, msg)      \
    do {                                                         \
        try {                                                    \
            statement;                                           \
            ASSERT_TRUE(false) << "The statement should throw!"; \
        } catch (exception_t & e) {                              \
            ASSERT_STREQ(e.what(), msg);                         \
        }                                                        \
    } while (0)

namespace ti_test {

class TmpFile {
  private:
    char filepath_holder[128];

  public:
    char *filepath;
    TmpFile();
    TmpFile(const TmpFile &f) = delete;
    TmpFile(TmpFile &&f) = delete;
    ~TmpFile();
};

struct LoggingTracerImpl;

class LoggingTracer{
    std::unique_ptr<LoggingTracerImpl> impl;
    public:
    // temporary set log level, and trace if not changelevelonly
    explicit LoggingTracer(const int level, bool changelevelonly);
    LoggingTracer(const LoggingTracer&) = delete;
    LoggingTracer(LoggingTracer&&) = delete;
    ~LoggingTracer();
    std::vector<std::string> log_content();
};

}  // namespace ti_test

#endif
