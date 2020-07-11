
#include "test.hpp" 
#include "network/protocol.hpp" 

using namespace thallium;
using namespace thallium::message;

TEST(NetworkProtocol, HeaderTest){

    Header h2{HeaderMessageType::heartbeat, 321432};

    char buf[header_size];
    header_to_string(h2, buf);
    Header h1 = string_to_header(buf);

    ASSERT_EQ(h1.msg_type, h2.msg_type);
    ASSERT_EQ(h1.body_length, h2.body_length);

}

TEST(NetworkProtocol, ZeroCopyBuffer){

    ZeroCopyBuffer b{64, 1};
    ASSERT_EQ(b.size(), 64);
    for (size_t i = 0; i < b.size(); i++) {
        ASSERT_EQ(b[i], 1);
        ASSERT_EQ(b.data()[i], 1);
    }

    CopyableBuffer & cb = b.to_copyable();
    CopyableBuffer cb1 = cb;

    ASSERT_EQ(cb1.size(), 64);
    for (size_t i = 0; i < b.size(); i++) {
        ASSERT_EQ(b[i], 1);
    }
}
