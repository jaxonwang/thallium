#include "test.hpp"
#include "runtime_protocol.hpp"

#include <unordered_set>

using namespace std;
using namespace thallium;

TEST(FirstConCookie, Basic){

    FirstConCookie c1;
    FirstConCookie c2;

    ASSERT_TRUE(c1 != c2);
    ASSERT_TRUE(c1 == c1);
    ASSERT_FALSE(c1 == c2);
    ASSERT_FALSE(c1 != c1);

    string s = c1.to_printable();

    FirstConCookie c3(s);

    ASSERT_EQ(s, c3.to_printable());
    ASSERT_EQ(c1, c3);

    s[5] = 'h';
    ASSERT_THROW_WITH(FirstConCookie{s}, "string element should be hexdecimal");
    ASSERT_THROW_WITH(FirstConCookie{"123"}, "The cookie string length should be 32");

    // test hash
    unordered_set<size_t> set;
    for (int i = 0; i < 1024 ; i++) {
        FirstConCookie tmp;
        size_t h = hash<FirstConCookie>()(tmp);
        ASSERT_TRUE(set.count(h) == 0); // no conflict
        set.insert(h);
    }

}
