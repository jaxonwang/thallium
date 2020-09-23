
#include "network/protocol.hpp"
#include "test.hpp"

using namespace std;
using namespace thallium;
using namespace thallium::message;

TEST(NetworkProtocol, HeaderTest) {
    Header h2{HeaderMessageType::heartbeat, 321432};

    char buf[header_size];
    header_to_string(h2, buf);
    Header h1 = string_to_header(buf);

    ASSERT_EQ(h1.msg_type, h2.msg_type);
    ASSERT_EQ(h1.body_length, h2.body_length);
}

TEST(NetworkProtocol, ZeroCopyBuffer) {
    ZeroCopyBuffer b{64, 1};
    ASSERT_EQ(b.size(), 64);
    for (size_t i = 0; i < b.size(); i++) {
        ASSERT_EQ(b[i], 1);
        ASSERT_EQ(b.data()[i], 1);
    }

    CopyableBuffer& cb = b.copyable_ref();
    CopyableBuffer cb1 = cb;

    ASSERT_EQ(cb1.size(), 64);
    for (size_t i = 0; i < b.size(); i++) {
        ASSERT_EQ(b[i], 1);
    }

    ZeroCopyBuffer cb2(move(b));
    ASSERT_EQ(cb2.size(), 64);
    ASSERT_EQ(b.size(), 0);
    for (size_t i = 0; i < cb2.size(); i++) {
        ASSERT_EQ(cb2[i], 1);
    }
}

struct t1 {
    int a;
    char b;
    unsigned short c;
    template <class A>
    void serializable(A& ar) {
        ar& a& b& c;
    }
};

TEST(NetworkProtocol, BuildAndCast) {
    t1 foo0{-1, 'f', 33};
    unsigned int foo1 = 0xffffffff;
    double pi = 3.14;
    char foo2 = 'k';
    unsigned int arr[123];
    for (int i = 0; i < 123; i++) {
        arr[i] = i;
    }
    long long foo3 = 5735873201498213;

    auto buf = message::build<message::ZeroCopyBuffer>(foo0, foo1, pi, foo2,
                                                       arr, foo3);
    ASSERT_EQ(buf.size(), Serializer::real_size(foo0) + sizeof(foo1) +
                              sizeof(pi) + sizeof(foo2) + sizeof(arr) +
                              sizeof(foo3));
    message::ReadOnlyBuffer b2(buf.data(), buf.size());
    t1 bar0;
    int bar1;
    double pi1;
    char bar2;
    unsigned int arr1[123];
    long long bar3;
    size_t ret = message::cast(b2, bar0, bar1, pi1, bar2, arr1, bar3);
    ASSERT_EQ(foo0.a, bar0.a);
    ASSERT_EQ(foo0.b, bar0.b);
    ASSERT_EQ(foo0.c, bar0.c);
    ASSERT_EQ(foo1, bar1);
    ASSERT_EQ(pi, pi1);
    ASSERT_EQ(foo2, bar2);
    ASSERT_EQ(foo3, bar3);
    for (int i = 0; i < 123; i++) {
        ASSERT_EQ(arr[i], arr1[i]);
    }
    ASSERT_EQ(ret, b2.size());
    char arr3[1234];
    ASSERT_THROW_WITH(
        message::cast(buf, foo0, arr3),
        "Remaining buf (size: 513) not enough to convert 1234 size value.");

    u_int32_t hafdf = 495121;
    auto buf2 = message::build<message::ZeroCopyBuffer>(hafdf);
    u_int32_t hafdf1;
    message::cast(buf2, hafdf1);
    ASSERT_EQ(hafdf1, hafdf);

    for (int i = 0; i < 1234; i++) {
        arr3[i] = i % 256;
    }

    message::ReadOnlyBuffer frombuf(arr3, 1234);

    auto buf1 = message::build<message::ZeroCopyBuffer>(foo1, frombuf, foo3);

    int baz1;
    long long baz3;
    message::ZeroCopyBuffer tobuf;
    message::cast(buf1, baz1, tobuf, baz3);
    ASSERT_EQ(tobuf.size(), 1234);
    ASSERT_EQ(foo1, baz1);
    for (int i = 0; i < 1234; i++) {
        ASSERT_EQ(arr3[i], tobuf.data()[i]);
    }
    ASSERT_EQ(foo3, baz3);

    auto buf3 =
        message::build<message::ZeroCopyBuffer>(frombuf, frombuf, frombuf);
    message::ZeroCopyBuffer tobuf0;
    message::ZeroCopyBuffer tobuf1;
    message::ZeroCopyBuffer tobuf2;
    message::cast(buf3, tobuf0, tobuf1, tobuf2);
    for (int i = 0; i < 1234; i++) {
        ASSERT_EQ(arr3[i], tobuf0.data()[i]);
        ASSERT_EQ(arr3[i], tobuf1.data()[i]);
        ASSERT_EQ(arr3[i], tobuf2.data()[i]);
    }

    ASSERT_THROW_WITH(message::cast(buf1, foo1, tobuf0, foo3, tobuf0),
            "Remaining buf (size: 0) not enough to convert 8 size value.");
}
