#include "network.hpp"

#include <cstring>
#include <vector>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

bool valid_hostname(const char *hostname) {
    if (!hostname) return false;
    if (strlen(hostname) > 255) return false;

    auto labels = string_split<vector>(hostname, ".");
    for (auto &label : labels) {
        if (label.size() == 0 || label.size() > 63)
            return false;  // label length should be 1-63

        // label chars must be "-" or digits or letters
        for (auto &c : label) {
            if (!(is_digit(c) || is_letter(c) || c == '-')) return false;
        }
        // should not start with "-"
        if (label[0] == '-') return false;
    }
    return true;
}

bool valid_port(const int port) { return (port > 0 && port < 65536); }

int port_to_int(const char *port) {
    if (!port) return -1;
    int p;
    for (const char *pos = port; *pos != 0; pos++) {
        if (!is_digit(*pos)) return -1;
    }
    try {
        p = std::stoi(port);
    } catch (exception &e) {
        return -1;
    }
    if (valid_port(p)) return p;
    return -1;
}

_THALLIUM_END_NAMESPACE
