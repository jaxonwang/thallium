
#include <iostream>
#include <string>

#include "common.hpp"
#include "test.hpp"

struct structtest {
    std::string value;
    structtest(const std::string &s) : value(s) {}
};
std::ostream &operator<<(std::ostream &os, const structtest &t) {
    os << t.value;
    return os;
}

TEST(FormatTest, FormatFunction) {
    using namespace thallium;
    using string = std::string;

    string t1 = "hello world!";
    ASSERT_TRUE(format(t1) == t1);
    ASSERT_TRUE(format("{} world!", "hello") == t1);
    ASSERT_TRUE(format("hello world{}", "!") == t1);
    ASSERT_TRUE(format("{} world{}", "hello", "!") == t1);
    ASSERT_TRUE(format("hello world{}", "!") == t1);
    ASSERT_TRUE(format("hello {}!", "world") == t1);
    ASSERT_TRUE(format("hello {}{}", "world", "!") == t1);
    ASSERT_TRUE(format("{} {}{}", "hello", "world", "!") == t1);
    string t2 = "{hello world!}";
    ASSERT_TRUE(format("{{{} {}", "hello", "world!}") == t2);
    ASSERT_TRUE(format("{} {}}}", "{hello", "world!") == t2);
    ASSERT_TRUE(format("{{{} {}}}", "hello", "world!") == t2);
    string t3 = "{}hello world!{}";
    ASSERT_TRUE(format("{{}}{} {}{{}}", "hello", "world!") == t3);
    string t4 = "{{hello world!}}";
    ASSERT_TRUE(format("{{{{{} {}}}}}", "hello", "world!") == t4);
    string t5 = "}hello world!{";
    ASSERT_TRUE(format("}}{} {}{{", "hello", "world!") == t5);
    string t6 = "hello{ world!";
    ASSERT_TRUE(format("{}{{ {}", "hello", "world!") == t6);
    string t7 = "hello} world!";
    ASSERT_TRUE(format("{}}} {}", "hello", "world!") == t7);
    ASSERT_TRUE(format("{}{}{}{}{} {}{}{}{}{}!", "h", "e", "l", "l", "o", "w", "o",
                   "r", "l", "d") == t1);

    ASSERT_TRUE(format("{} {} {} {} {} {}", 1, 2.1, -1, 0xff, 'c', 0) ==
            "1 2.1 -1 255 c 0");

    ASSERT_TRUE(format("{} world{}", structtest{"hello"}, "!") == t1);
    ASSERT_THROW(format("hello world!{"), ti_exception::format_error);
    ASSERT_THROW(format("}hello world!"), ti_exception::format_error);
    ASSERT_THROW(format("{hello world!"), ti_exception::format_error);
    ASSERT_THROW(format("hello world!}"), ti_exception::format_error);
    ASSERT_THROW(format("{hello} world!"), ti_exception::format_error);
    ASSERT_THROW(format("hello {world!}"), ti_exception::format_error);
    ASSERT_THROW(format("hello {world!"), ti_exception::format_error);
    ASSERT_THROW(format("hello }world!"), ti_exception::format_error);
    ASSERT_THROW(format("hello {{}world!", "ha"),
                      ti_exception::format_error);
    ASSERT_THROW(format("hello {}}world!", "ha"),
                      ti_exception::format_error);

    ASSERT_THROW(format("hello {}world!"), ti_exception::format_error);
    ASSERT_THROW(format("hello {}{}world!", "ha1"),
                      ti_exception::format_error);
    ASSERT_THROW(format("hello world!", "ha1"),
                      ti_exception::format_error);
    ASSERT_THROW(format("hello {}world!", "ha1", "ha2"),
                      ti_exception::format_error);
    ASSERT_THROW(format("hello world!{}", "ha1", "ha2"),
                      ti_exception::format_error);
    ASSERT_THROW(format("{}hello world!", "ha1", "ha2"),
                      ti_exception::format_error);
}
