#include "coordinator.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "exception.hpp" 

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

host_file_entry parse_host_file_entry(const string &line) {
    auto splited = string_split<vector>(line, ":");
    host_file_entry entry;
    auto len = splited.size();

    using namespace ti_exception;
    if (len < 1) {
        TI_RAISE(bad_user_input("line is emtpy! Should not reach here."));
    } else if (len == 2) {
        bool valid = true;
        int process_num;
        try{
            process_num = stoi(string_trim(splited[1]));  // may raise
        }catch(const std::invalid_argument &e){
            valid = false;
        }catch(const std::out_of_range &e){
            valid = false;
        }
        // TODO define MAX_PARALLEL
        if (process_num <= 0) {
            valid = false;
        }
        if(!valid)
            TI_RAISE(bad_user_input(format("Invalid process number: {}", line)));
        entry.process_num = process_num;
    } else if (len == 1) {
        entry.process_num = 1;
    } else {
        TI_RAISE(bad_user_input(format("Invalid format: {}", line)));
    }
    // check host name is valid
    entry.hostname = string_trim(splited[0]);
    if (entry.hostname.size() == 0) {
        TI_RAISE(bad_user_input(format("Invalid hostname: {}", line)));
    }
    return entry;
}

vector<host_file_entry> read_host_file(const char *file_path) {
    vector<host_file_entry> hosts;
    ifstream host_f;
    host_f.open(file_path);

    string in_str;

    for (; getline(host_f, in_str);) {
        if(string_trim(in_str).size()==0)
            continue; //empty line
        hosts.push_back(parse_host_file_entry(in_str));
    }
    if(hosts.size()==0)
        TI_RAISE(ti_exception::bad_user_input("Invalid hostfile"));

    host_f.close();
    return hosts;
}

_THALLIUM_END_NAMESPACE
