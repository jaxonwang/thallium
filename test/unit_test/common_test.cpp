
#include <iostream>
#include <vector>
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

TEST(StringTest, StringJoin){
    string sep0 {","};
    vector<string> strlst0 = {"0", "1", "2", "3", "4"};

    string ret = thallium::string_join(strlst0, sep0);
    ASSERT_EQ(ret, "0,1,2,3,4");
    ret = thallium::string_join(vector<string>{}, sep0);
    ASSERT_EQ(ret, "");
    ret = thallium::string_join(strlst0, "");
    ASSERT_EQ(ret, "01234");
    ret = thallium::string_join(vector<string>{}, "");
    ASSERT_EQ(ret, "");

    vector<string> strlst1 = {"", "0", "", "1", "2", "3", "4", ""};
    ret = thallium::string_join(strlst1, sep0);
    ASSERT_EQ(ret, ",0,,1,2,3,4,");
    ret = thallium::string_join(strlst1, "");
    ASSERT_EQ(ret, "01234");

    vector<string> strlst3 = {"", "", "", "", "", ""};
    ret = thallium::string_join(strlst3, sep0);
    ASSERT_EQ(ret, ",,,,,");

    vector<string> strlst4 = {"hello", "world", "ha", "ha"};
    ret = thallium::string_join(strlst4, "__");
    ASSERT_EQ(ret, "hello__world__ha__ha");

    vector<wstring> strlst2;
    for (int i = 0; i < 5; i++) {
        strlst2.push_back(to_wstring(i));
    }
    wstring sep2{L","};
    wstring ret2 = thallium::string_join(strlst2, sep2);
    ASSERT_EQ(ret2, wstring(L"0,1,2,3,4"));

}

TEST(StringTest, StringTrim){
    using namespace thallium;
    string s0{""};
    ASSERT_EQ("", string_trim(s0));

    ASSERT_EQ("", string_trim(" "));

    ASSERT_EQ("123", string_trim(string{" 123"}));
    ASSERT_EQ("123", string_trim(string{"123 "}));

    ASSERT_EQ("123", string_trim("123  "));
    ASSERT_EQ("123", string_trim("  123 "));
    ASSERT_EQ("123", string_trim("123\t  "));
    ASSERT_EQ("123", string_trim("  \n123 "));
    ASSERT_EQ("123", string_trim("  \n\r123\t "));

    ASSERT_EQ("123321", string_trim("123321", ""));
    ASSERT_EQ("2332", string_trim("123321", "1"));
    ASSERT_EQ("33", string_trim("123321", "21"));

    ASSERT_EQ(L"123321", string_trim(L"123321", L""));
    ASSERT_EQ(L"123", string_trim(L"  \n\r123\t "));
    ASSERT_EQ(L"2332", string_trim(L"123321", L"1"));
    ASSERT_EQ(L"33", string_trim(L"123321", L"21"));

}
