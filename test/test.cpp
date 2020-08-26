#include "test.hpp"

#include <cstdio>
#include <algorithm>
#include <fstream>

#ifdef __linux__
    #include <cstdlib>
#endif

#include <unistd.h>

namespace ti_test {

TmpFile::TmpFile():filepath(filepath_holder){
    const char pattern[] ="justatempfilenameXXXXXX";
    std::copy(pattern, pattern + sizeof(pattern), filepath_holder);

    int ret = mkstemp(filepath_holder);

    if(!ret)
        throw std::runtime_error("Can not find a tmp filename");

    close(ret);
}

TmpFile::~TmpFile(){
    std::remove(filepath);
}

};  // namespace ti_test
