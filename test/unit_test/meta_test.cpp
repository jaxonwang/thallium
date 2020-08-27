#include <vector>

#include "test.hpp"
#include "logging.hpp"

using namespace std;
using namespace thallium;

TEST(Meta, LoggingTracer){
    logging_init(0);
    ti_test::LoggingTracer t(2, false);
    vector<string> strs = {"just", "test", "hello", "world", "!"};

    for (size_t i = 0; i < strs.size(); i++) {
       TI_DEBUG("just test");
       TI_INFO("just test");
       TI_WARN(to_string(i));
    }

    vector<string> logs = t.log_content();

    ASSERT_EQ(logs.size(), strs.size());
    for (size_t i = 0; i < strs.size(); i++) {
        ASSERT_TRUE(logs[i].find(strs[i])!=string::npos);
    }
}

TEST(Meta, LoggingTracerCleanUp){
    int level = get_global_manager()->get_level_filter().min_level;
    ASSERT_EQ(level, 0);
}
