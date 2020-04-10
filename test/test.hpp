#ifndef _THALLIUM_TEST
#define _THALLIUM_TEST

#include "gtest/gtest.h"

#define ASSERT_THROW_WITH(x, y)                                   \
    do {                                                          \
        try {                                                     \
            x;                                                    \
            ASSERT_TRUE(false) << "The expression should throw!"; \
        } catch (std::exception & e) {                            \
            ASSERT_STREQ(e.what(), y);                            \
        }                                                         \
    } while (0)

#endif
