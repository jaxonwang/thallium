#include "utils.hpp"

#include <thread>
#include <vector>

#include "test.hpp"

using namespace std;
using namespace thallium;

TEST(ThreadWrapper, Join) {
    bool touched = false;

    {
        thread_join_wrapper t{thread{[&]() { touched = true; }}};
    }
    ASSERT_EQ(touched, true);
}
