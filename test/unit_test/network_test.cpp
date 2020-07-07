#include "network/network.hpp"

#include "test.hpp"
using namespace thallium;

TEST(UtilsTest, ValidHostname) {
    ASSERT_TRUE(valid_hostname("abcdefg"));
    ASSERT_TRUE(valid_hostname("abcdefg.fdsaf"));
    ASSERT_TRUE(valid_hostname("a314bcdefg.fdsaf"));
    ASSERT_TRUE(valid_hostname("a314-bcdefg.fd-saf"));
    ASSERT_TRUE(valid_hostname("123-bcdefg.fd-saf.123"));
    ASSERT_TRUE(valid_hostname("123-bcdefg.fd-saf.123"));
    ASSERT_TRUE(valid_hostname("123.123-bcdefg.fd-saf.123"));
    ASSERT_TRUE(valid_hostname("456.123.123-bcdefg.fd-saf.123"));

    ASSERT_FALSE(valid_hostname(
        "1234123412341234123412341234123412341234123412341234123412341234"));
    char too_long[260] = {};
    for (int i = 0; i < 259; i++) {
        too_long[i] = 'a';
    }
    ASSERT_FALSE(valid_hostname(too_long));
    ASSERT_FALSE(valid_hostname(nullptr));
    ASSERT_FALSE(valid_hostname(""));
    ASSERT_FALSE(valid_hostname("."));
    ASSERT_FALSE(valid_hostname("dsaf..dsafsd"));
    ASSERT_FALSE(valid_hostname("-dsaf..dsafsd"));
    ASSERT_FALSE(valid_hostname(".dsaf.dsafsd"));
    ASSERT_FALSE(valid_hostname("dsaf.dsafsd."));
    ASSERT_FALSE(valid_hostname("dsaf.dsafsd.43*^%)"));
}

TEST(UtilsTest, ValidPort) {
    ASSERT_EQ(1235, port_to_int("1235"));
    ASSERT_EQ(-1, port_to_int(nullptr));
    ASSERT_EQ(1, port_to_int("1"));
    ASSERT_EQ(65535, port_to_int("65535"));
    ASSERT_EQ(-1, port_to_int("0"));
    ASSERT_EQ(-1, port_to_int("-123"));
    ASSERT_EQ(-1, port_to_int("a1234"));
    ASSERT_EQ(-1, port_to_int("1234k"));
    ASSERT_EQ(-1, port_to_int("65536"));
    ASSERT_EQ(-1, port_to_int("165536"));
}
