#include "logging.hpp"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "common.hpp"

using namespace std;
using namespace thallium;

void S_ASSERT(int ret) {
    if (ret == -1) {
        perror("Test quit with fatal error:");
        exit(1);
    }
}

// this should be only called in check body
template<class T1, class T2>
void ASSERT_EQ(const T1 &t1, const T2 &t2, const string &msg){
    if(t1 != t2){
        cout << msg << endl;
        cout << t1 << endl;
        cout << t2 << endl;
        exit(1);
    }
}

template<class T1>
void ASSERT_TRUE(const T1 &t1, const string &msg){
    if(!t1){
        cout << msg << endl;
        exit(1);
    }
}

const char *level_tbl[5] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

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

const int log_num = 10000;

void test_body() {
    logging_init(LOG_LEVEL_INFO_NUM);

    TI_DEBUG("this should not be logged.");

    for (int i = 0; i < log_num; i++) {
        TI_INFO(format("msg{}", i));
    }
}

void check_body() {
    for (int i = 0; i < log_num; i++) {
        std::string tmp;
        ASSERT_TRUE(std::getline(cin, tmp), "Expected to receive more output!");
        ASSERT_EQ(get_level(tmp), LOG_LEVEL_INFO_NUM, "Message level should be INFO.");
        string expected_msg = format("msg{}", i);
        ASSERT_EQ(get_msg_from_record_string(tmp), expected_msg, "Diffrent msg receved.");
    }
}


int main() {
    int pipe_fd[2]; // pipe_fd[0] for read, pipe_fd[1] for write
    S_ASSERT(pipe(pipe_fd));

    if(!fork()){
        S_ASSERT(dup2(pipe_fd[1], STDOUT_FILENO));
    test_body();
    }
    else{
        S_ASSERT(dup2(pipe_fd[0], STDIN_FILENO));
        check_body();
    }
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    return 0;
}
