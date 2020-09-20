#ifndef _THALLIUM_WORKER_HEADER
#define _THALLIUM_WORKER_HEADER

#include "network/network.hpp"


struct WorkerDataServerInfo{
    unsigned short port;
    std::string addr;

    template <class Archive>
    void serializable(Archive& ar) {
        ar& port;
        ar& addr;
    }
};

struct WorkerInfo{
    WorkerDataServerInfo data_server;
    template <class Archive>
    void serializable(Archive& ar) {
        ar& data_server;
    }
};

#endif

