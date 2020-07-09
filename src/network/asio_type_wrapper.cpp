#include "asio_type_wrapper.hpp"
#include "network.hpp"

#include <string>

_THALLIUM_BEGIN_NAMESPACE

using namespace std;
real_addr_type get_host_by_name(const char *hostname, error_code& ec){
    // return boost::asio::ip::address_v4::make_address_v4(hostname, ec);
}

string ti_socket_t::to_string() const {
    return addr.to_string() + ":" + std::to_string(port);
}

_THALLIUM_END_NAMESPACE
