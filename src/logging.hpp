#ifndef _THALLIUM_LOGGING_HEADER
#define _THALLIUM_LOGGING_HEADER

#include <atomic>
#include <thread>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>

#include "common.hpp"
#include "concurrent.hpp"

#define LOG_LEVEL_DEBUG_NUM 0
#define LOG_LEVEL_INFO_NUM 1
#define LOG_LEVEL_WARN_NUM 2
#define LOG_LEVEL_ERROR_NUM 3
#define LOG_LEVEL_FATAL_NUM 4

#ifdef LOG_LEVEL_DEBUG
#define LOG_LEVEL_NUM LOG_LEVEL_DEBUG_NUM
#elif defined(LOG_LEVEL_INFO)
#define LOG_LEVEL_NUM LOG_LEVEL_INFO_NUM
#elif defined(LOG_LEVEL_WARN)
#define LOG_LEVEL_NUM LOG_LEVEL_WARN_NUM
#elif defined(LOG_LEVEL_ERROR)
#define LOG_LEVEL_NUM LOG_LEVEL_ERROR_NUM
#elif defined(LOG_LEVEL_FATAL)
#define LOG_LEVEL_NUM LOG_LEVEL_FATAL_NUM
#else
#define LOG_LEVEL_NUM LOG_LEVEL_INFO_NUM
#endif

#define TI_DEBUG(msg)                                             \
    do {                                                          \
        _log(LOG_LEVEL_DEBUG_NUM, __FILE__, __LINE__, move(msg)); \
    } while (0)

#define TI_INFO(msg)                                             \
    do {                                                         \
        _log(LOG_LEVEL_INFO_NUM, __FILE__, __LINE__, move(msg)); \
    } while (0)

#define TI_WARN(msg)                                             \
    do {                                                         \
        _log(LOG_LEVEL_WARN_NUM, __FILE__, __LINE__, move(msg)); \
    } while (0)

#define TI_ERROR(msg)                                             \
    do {                                                          \
        _log(LOG_LEVEL_ERROR_NUM, __FILE__, __LINE__, move(msg)); \
    } while (0)

#define TI_FATAL(msg)                                             \
    do {                                                          \
        _log(LOG_LEVEL_FATAL_NUM, __FILE__, __LINE__, move(msg)); \
    } while (0)

_THALLIUM_BEGIN_NAMESPACE

class AsyncLogger {
  private:
    SenderSideLockQueue<std::string> record_queue;
    std::ostream &_dest;

  public:
    AsyncLogger(std::ostream &);
    AsyncLogger(const AsyncLogger &) = delete;
    AsyncLogger(AsyncLogger &&) = delete;
    void flush_one();
    void flush_all();
    void insert_record(const std::string &log);
    void insert_record(std::string &&log);
};

class LevelFilter {
  public:
    const int min_level;
    LevelFilter(const int level);
    bool filter(int level);
};

class GlobalLoggerManager {
  private:
    std::atomic_bool done;
    std::unique_ptr<std::ostream> _ostream_ptr;
    std::unique_ptr<AsyncLogger> _logger_ptr;
    LevelFilter _level_filter;
    std::thread deamon;

  public:
    GlobalLoggerManager(int level);

    GlobalLoggerManager(int level, const char *file_path);

    GlobalLoggerManager(const GlobalLoggerManager &) = delete;
    GlobalLoggerManager(GlobalLoggerManager &&) = delete;
    ~GlobalLoggerManager();

    AsyncLogger &get_logger();

    LevelFilter &get_level_filter();

    void loop_body();

    void stop();
};

class Record {
    int level;
    int line_num;
    std::thread::id thread_id;
    const char *file_name;
    const std::string msg;
    std::chrono::system_clock::time_point logging_time;

  public:
    Record(const int level, const char *file_name, const int line_num,
           const std::string &&msg);
    std::string format();
    const char *level_name(int level_num);

};

void logging_init(int level, const char * file_path = nullptr); 

void _log(int level, const char *file_name, const int line_num, const std::string &&msg);

std::unique_ptr<GlobalLoggerManager> &get_global_manager();

_THALLIUM_END_NAMESPACE

#endif
