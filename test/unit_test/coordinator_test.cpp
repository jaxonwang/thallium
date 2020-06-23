#include "coordinator.hpp"

#include <fstream>
#include <iostream>

#include "test.hpp"

using namespace thallium;
using namespace thallium::ti_exception;
using namespace std;

_THALLIUM_BEGIN_NAMESPACE
bool operator==(const host_file_entry &a, const host_file_entry &b) {
    return (a.hostname == b.hostname && a.process_num == b.process_num);
}
_THALLIUM_END_NAMESPACE

TEST(Host, ParseHostFileEntry) {
    host_file_entry e;
    e = {"justhost", 4};
    ASSERT_EQ(e, parse_host_file_entry("justhost:4"));
    ASSERT_EQ(e, parse_host_file_entry("justhost : 4"));
    ASSERT_EQ(e, parse_host_file_entry(" justhost : 4 "));
    ASSERT_EQ(e, parse_host_file_entry("   justhost   :   4   "));
    ASSERT_EQ(e, parse_host_file_entry("\tjusthost : 4\n"));
    ASSERT_EQ(e, parse_host_file_entry("\tjusthost\t:\t4\n"));

    ASSERT_THROW(parse_host_file_entry("\tjusthost\t::\t4\n"), bad_user_input);
    ASSERT_THROW(parse_host_file_entry(""), bad_user_input);
    ASSERT_THROW_WITH_TYPE(parse_host_file_entry(":3"), bad_user_input,
                           "Invalid hostname: :3");
    ASSERT_THROW(parse_host_file_entry("host:"), bad_user_input);
    ASSERT_THROW(parse_host_file_entry(":"), bad_user_input);

    ASSERT_THROW(parse_host_file_entry("host::123"), bad_user_input);
    ASSERT_THROW(parse_host_file_entry(":host:123"), bad_user_input);
    ASSERT_THROW_WITH_TYPE(parse_host_file_entry("host:123:"), bad_user_input,
                           "Invalid format: host:123:");
    ASSERT_THROW_WITH_TYPE(parse_host_file_entry("host:-123"), bad_user_input,
                           "Invalid process number: host:-123");

    ASSERT_THROW_WITH_TYPE(parse_host_file_entry("host:0"), bad_user_input,
                           "Invalid process number: host:0");
    ASSERT_THROW_WITH_TYPE(parse_host_file_entry("host:abc"), bad_user_input,
                           "Invalid process number: host:abc");
}

TEST(Host, ReadHostFile) {
    ti_test::TmpFile tmp;
    const char *c1 = "  \n\n host1: 1\n    host2: 2\n\thost3:   3   \r\nhost4: 4\n\n";

    ASSERT_THROW_WITH_TYPE(read_host_file(tmp.filepath), bad_user_input,
                           "Invalid hostfile");

    vector<host_file_entry> entries = {
        {"host1", 1}, {"host2", 2}, {"host3", 3}, {"host4", 4}};
    ofstream f(tmp.filepath);
    f << c1;
    f.close();

    auto ret = read_host_file(tmp.filepath);
    ASSERT_EQ(entries.size(), ret.size());
    for (int i = 0; i < (int)ret.size(); i++) {
       ASSERT_EQ(ret[i], entries[i]);
    }
}
