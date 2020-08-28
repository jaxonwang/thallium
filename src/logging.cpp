#include "logging.hpp"

#include <mutex>
#include <unordered_map>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

AsyncLogger::AsyncLogger(ostream &os) : record_queue(), _dest(os) {}

void AsyncLogger::insert_record(const string &log_record) {
    record_queue.send(log_record);
}

void AsyncLogger::insert_record(string &&log_record) {
    record_queue.send(move(log_record));
}

void AsyncLogger::flush_one() {  // will block
    string tmp;
    record_queue.receive(tmp);
    if (tmp.size() > 0) {  // empty str is a marker to stop
        _dest << tmp;
    }
}

void AsyncLogger::flush_all() {  // will not block
    string tmp;
    while (record_queue.try_receive(tmp)) {  // try until nothing is left
        if (tmp.size() > 0) {
            _dest << tmp;
        }
    }
}

LevelFilter::LevelFilter(const int level) : min_level(level) {}

bool LevelFilter::filter(int level) { return level >= min_level; }

GlobalLoggerManager::GlobalLoggerManager(int level)
    : done(false),
      _ostream_ptr(nullptr),
      _logger_ptr(new AsyncLogger(std::cout)),  // use cout as default output
      _level_filter(level),
      deamon([this]() { this->loop_body(); }) {}

GlobalLoggerManager::GlobalLoggerManager(int level, const char *file_path)
    : done(false), _level_filter(level) {
    ofstream *s_ptr = new ofstream{file_path, ios_base::app};
    if (!s_ptr->is_open()) {
        cerr << "Can not open file:" << file_path << ". Will log to stdout."
             << endl;
        delete s_ptr;
        _logger_ptr.reset(new AsyncLogger(std::cout));
    } else {
        _ostream_ptr.reset(s_ptr);
        _logger_ptr.reset(new AsyncLogger(*_ostream_ptr));
    }
    thread tmp{[this]() {
        this->loop_body();
    }};  // use tmp variable to avoid uncertain copy elision
    deamon = move(tmp);
}

void GlobalLoggerManager::flush_records() {
    _logger_ptr->flush_all();
    _ostream_ptr->flush();
}

GlobalLoggerManager::~GlobalLoggerManager() {
    // does not exception free...
    stop();
    if (deamon.joinable()) {
        deamon.join();
    }
}

AsyncLogger &GlobalLoggerManager::get_logger() { return *_logger_ptr; }

LevelFilter &GlobalLoggerManager::get_level_filter() { return _level_filter; }

void GlobalLoggerManager::loop_body() {
    while (!done) {
        _logger_ptr->flush_one();
    }
    _logger_ptr->flush_all();  // when waken by stop(), there might be something
                               // left in queue
}

void GlobalLoggerManager::stop() {
    done = true;
    _logger_ptr->insert_record(
        string{});  // insert a empty record to wake the logger
}

const char *get_file_name(const char *path) {
    // get the file name for __FILE__ macro
    //  should be thread safe
    static thread_local unordered_map<const char *, const char *> dict;
    if (dict.count(path) == 0) {
        const char *pos = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
        dict[path] = pos;
    }
    return dict[path];
}

Record::Record(const int level, const char *file_name, const int line_num,
               const string &&msg)
    : level(level),
      line_num(line_num),
      thread_id(this_thread::get_id()),
      file_name(get_file_name(file_name)),
      msg(move(msg)),
      logging_time(chrono::system_clock::now()) {}

string Record::format() {
    stringstream ss;
    // date time
    time_t lg_time_t = chrono::system_clock::to_time_t(logging_time);
    std::tm local_tm;
    localtime_r(&lg_time_t, &local_tm);  // thread safe
    char str_time[128] = {0};
    const char *date_format = "%Y-%m-%d %H:%M:%S.";
    strftime(str_time, sizeof(str_time), date_format, &local_tm);
    int ms = chrono::duration_cast<chrono::milliseconds>(
                 logging_time.time_since_epoch())
                 .count() %
             1000;
    // level

    ss << str_time << ms;
    ss << " " << level_name(level) << " ";
    ss << "tid:" << thread_id << " ";
    ss << "in " << file_name << ":" << line_num;
    ss << " " << msg << endl;
    return ss.str();
}

const char *Record::level_name(int level_num) {
    switch (level_num) {
        case LOG_LEVEL_DEBUG_NUM:
            return "DEBUG";
        case LOG_LEVEL_INFO_NUM:
            return "INFO";
        case LOG_LEVEL_WARN_NUM:
            return "WARN";
        case LOG_LEVEL_ERROR_NUM:
            return "ERROR";
        case LOG_LEVEL_FATAL_NUM:
            return "FATAL";
        default:
            throw logic_error("bad level");
    };
}

// global storage of mng
static unique_ptr<GlobalLoggerManager> mng;

unique_ptr<GlobalLoggerManager> &get_global_manager() { return mng; }

AsyncLogger &get_global_logger() { return mng->get_logger(); }

LevelFilter &get_global_level_filter() { return mng->get_level_filter(); }

void logging_init(int level, const char *log_file_path) {
    if (log_file_path)
        mng.reset(new GlobalLoggerManager{level, log_file_path});
    else
        mng.reset(new GlobalLoggerManager{level});
}

void _log(int level, const char *file_name, const int line_num,
          const string &&msg) {
    if (!get_global_level_filter().filter(level)) return;
    string record_s = Record(level, file_name, line_num, move(msg)).format();
    get_global_logger().insert_record(move(record_s));
}

_THALLIUM_END_NAMESPACE
