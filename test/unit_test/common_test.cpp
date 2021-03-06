
#include "common.hpp"

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "test.hpp"

using namespace thallium;
using namespace std;

struct structtest {
    std::string value;
    structtest(const std::string &s) : value(s) {}
};
std::ostream &operator<<(std::ostream &os, const structtest &t) {
    os << t.value;
    return os;
}
TEST(VariadicSum, Base) {
    ASSERT_EQ(variadic_sum(1, 2, 3, 4, 5), 15);
    ASSERT_EQ(variadic_sum(1), 1);
}

TEST(FormatTest, FormatFunction) {
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
    ASSERT_TRUE(format("{}{}{}{}{} {}{}{}{}{}!", "h", "e", "l", "l", "o", "w",
                       "o", "r", "l", "d") == t1);

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
    ASSERT_THROW(format("hello {{}world!", "ha"), ti_exception::format_error);
    ASSERT_THROW(format("hello {}}world!", "ha"), ti_exception::format_error);

    ASSERT_THROW(format("hello {}world!"), ti_exception::format_error);
    ASSERT_THROW(format("hello {}{}world!", "ha1"), ti_exception::format_error);
    ASSERT_THROW(format("hello world!", "ha1"), ti_exception::format_error);
    ASSERT_THROW(format("hello {}world!", "ha1", "ha2"),
                 ti_exception::format_error);
    ASSERT_THROW(format("hello world!{}", "ha1", "ha2"),
                 ti_exception::format_error);
    ASSERT_THROW(format("{}hello world!", "ha1", "ha2"),
                 ti_exception::format_error);
}

TEST(StringTest, StringJoin) {
    string sep0{","};
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

TEST(StringTest, StringTrim) {
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

TEST(StringTest, StringSplit) {
    using namespace thallium;
    ASSERT_EQ(vector<string>({"abc"}), string_split<vector>("abc", ""));
    ASSERT_EQ(vector<string>({"abc"}), string_split<vector>("abc", "d"));
    ASSERT_EQ(vector<string>({""}), string_split<vector>("", "d"));
    ASSERT_EQ(vector<string>({""}), string_split<vector>("", ""));

    ASSERT_EQ(vector<string>({"1", "2", "3", "4", "5"}),
              string_split<vector>("1 2 3 4 5", " "));
    ASSERT_EQ(vector<string>({"", "2", "3", "4", ""}),
              string_split<vector>(" 2 3 4 ", " "));
    ASSERT_EQ(vector<string>({"2", "", "4"}),
              string_split<vector>("2  4", " "));

    ASSERT_EQ(vector<string>({"1", "41", "4"}),
              string_split<vector>("12341234", "23"));
    ASSERT_EQ(vector<string>({"", "", ""}),
              string_split<vector>("12341234", "1234"));

    ASSERT_EQ(list<string>({"1", "41", "4"}),
              string_split<list>("12341234", "23"));
    ASSERT_EQ(list<string>({"", "", ""}),
              string_split<list>("12341234", "1234"));

    ASSERT_EQ(list<wstring>({L"1", L"41", L"4"}),
              string_split<list>(L"12341234", L"23"));
    ASSERT_EQ(list<wstring>({L"", L"", L""}),
              string_split<list>(L"12341234", L"1234"));

    ASSERT_EQ(list<string>({"12", "34", "56", "78"}),
              string_split<list>("12__34__56__78", "__"));
}

TEST(StringTest, CharTest) {
    // I ate too much
    using namespace thallium;
    string lower{"abcdefghijklmnopqrstuvwxyz"};
    string upper{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    string others{"!@#$%^&*()_+|~}{:\"?><\\=-`"};
    string digits{"1234567890"};

    for (auto &&c : lower) {
        ASSERT_TRUE(is_lowercase(c));
        ASSERT_TRUE(is_letter(c));
        ASSERT_FALSE(is_digit(c));
        ASSERT_FALSE(is_uppercase(c));
    }
    for (auto &&c : upper) {
        ASSERT_FALSE(is_lowercase(c));
        ASSERT_TRUE(is_letter(c));
        ASSERT_FALSE(is_digit(c));
        ASSERT_TRUE(is_uppercase(c));
    }
    for (auto &&c : digits) {
        ASSERT_FALSE(is_lowercase(c));
        ASSERT_FALSE(is_letter(c));
        ASSERT_TRUE(is_digit(c));
        ASSERT_FALSE(is_uppercase(c));
    }
    for (auto &&c : others) {
        ASSERT_FALSE(is_lowercase(c));
        ASSERT_FALSE(is_letter(c));
        ASSERT_FALSE(is_digit(c));
        ASSERT_FALSE(is_uppercase(c));
    }
}
