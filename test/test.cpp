#include "test.hpp"

#include <cstdio>
#include <fstream>

namespace ti_test {

TmpFile::TmpFile() {
    filepath = std::tmpnam(filepath_holder);
    if(!filepath)
        throw std::runtime_error("Can not find a tmp filename");

    // touch
    std::ofstream f;
    f.open(filepath);
    f.close();
}

TmpFile::~TmpFile(){
    std::remove(filepath);
}

};  // namespace ti_test
