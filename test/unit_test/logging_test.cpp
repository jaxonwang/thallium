#include "logging.hpp"
#include "common.hpp"

#include <iostream>
#include <ctime>
#include <thread>
#include <vector>
#include <cstdlib>

#include "test.hpp"

using namespace thallium;
using namespace std;

const char *level_tbl[5] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

TEST(Logging, RecordTest) {
    const char * msg = "justamessagefjdsal;utfiewqjgvladsn;";
    Record r1(1, __FILE__, __LINE__, msg);
    string ret = r1.format();
    
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int ms;

    auto slices = string_split<vector>(string_trim(ret), " ");
    sscanf(slices[0].c_str(), "%d-%d-%d", &year, &month, &day);
    sscanf(slices[1].c_str(), "%d:%d:%d.%d", &hour, &minute, &second, &ms);

    time_t now = time(nullptr);
    tm *now_tm = localtime(&now);

    ASSERT_EQ(year, now_tm->tm_year + 1900);
    ASSERT_EQ(month, now_tm->tm_mon+1);
    ASSERT_EQ(day, now_tm->tm_mday);
    ASSERT_EQ(hour, now_tm->tm_hour);
    ASSERT_EQ(minute, now_tm->tm_min);
    ASSERT_EQ(second, now_tm->tm_sec);
    ASSERT_TRUE(ms >= 0 && ms < 1000000);

    ASSERT_EQ(slices[2], "INFO");

    ASSERT_EQ(slices[3].rfind("tid", 0), 0); //start with tid
    ASSERT_EQ(slices[4], "in");

    const char * filename = "logging_test.cpp";
    auto file_and_line = string_split<vector>(slices[5], ":");
    ASSERT_EQ(file_and_line[0], filename);

    ASSERT_EQ(slices[6], msg);

    // loglevelname
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(string(r1.level_name(i)), level_tbl[i]);
    }
}

string get_msg_from_record_string(string &record){
    auto v = string_split<vector>(record, " ");
    return v[v.size()-1];
}

int get_level(string &record){
    auto v = string_split<vector>(record, " ");
    string &level = v[2];
    for (size_t i = 0; i < sizeof(level_tbl); i++) {
        if(level == level_tbl[i])
            return i;
    }
    throw runtime_error("no such record.");
}

TEST(Logging, GlobalLoggerManager){
    ti_test::TmpFile t_f{};
    auto g_mng = new GlobalLoggerManager{2, t_f.filepath};
    auto && logger = g_mng->get_logger();

    vector<const char*> msgs = {"msg1", "msg2", "msg3", "msg4", "fdsalfjlsdafksadl"};
    for (unsigned long i = 0; i < msgs.size(); i++) {
        logger.insert_record(Record{2, __FILE__, __LINE__, msgs[i]}.format());
    }
    delete g_mng; 
    string line;
    ifstream fs{t_f.filepath};
    int i = 0;
    for(; getline(fs, line); i++){
        ASSERT_EQ(get_msg_from_record_string(line), msgs[i]);
    }
    // this is a mistake I made, forget to check the loop is really executed to N times
    ASSERT_EQ(i, msgs.size()); 
}
