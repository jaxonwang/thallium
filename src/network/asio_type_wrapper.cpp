#include "asio_type_wrapper.hpp"
#include "logging.hpp"

#include <system_error>
#include <string>

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

string ti_socket_t::to_string() const {
    return addr.to_string() + ":" + std::to_string(port);
}

real_addr_type resolve(const string &s, std::error_code &e){
    ti_socket_t skt;
    // seems that there is no way to convert std::error_code to boost::system::error_code
    boost::system::error_code b_e;
    real_addr_type ret = boost::asio::ip::make_address_v4(s.c_str(), b_e);
    e = b_e;

    if(e){
        TI_ERROR(format("Trying to resolve host: {} error: {}", s, e.message()));
    }
    return ret;
}

_THALLIUM_END_NAMESPACE
