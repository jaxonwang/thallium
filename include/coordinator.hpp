#ifndef _THALLIUM_COORDINATOR
#define _THALLIUM_COORDINATOR

#include <vector>

#include "common.hpp"


_THALLIUM_BEGIN_NAMESPACE

namespace ti_exception {
class bad_user_input: public runtime_error {
  public:
    explicit bad_user_input(const string &what_arg) : runtime_error(what_arg) {}
};
}  // namespace ti_exception


struct host_file_entry{
    string hostname;
    int process_num;
};

host_file_entry parse_host_file_entry(const string &);

vector<host_file_entry> read_host_file(const char *);


_THALLIUM_END_NAMESPACE

#endif
