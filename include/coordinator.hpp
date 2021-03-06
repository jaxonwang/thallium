#ifndef _THALLIUM_COORDINATOR
#define _THALLIUM_COORDINATOR

#include <vector>
#include <functional>
#include <unordered_set>

#include "common.hpp"
#include "network/network.hpp"
#include "runtime_protocol.hpp"
#include "place.hpp"
#include "worker.hpp"


_THALLIUM_BEGIN_NAMESPACE

namespace ti_exception {
class bad_user_input: public std::runtime_error {
  public:
    explicit bad_user_input(const std::string &what_arg) : std::runtime_error(what_arg) {}
};
}  // namespace ti_exception


struct host_file_entry{
    std::string hostname;
    int process_num;
};
class CoordinatorServer;
using CoordServerBase = StateMachine<CoordinatorServer, ServerModel>;
class CoordinatorServer : public CoordServerBase {
    typedef std::unordered_set<FirstConCookie> cookie_set;
    cookie_set cookies;
    std::unordered_map<int, PlaceObj> workers;
    std::unordered_map<int, WorkerInfo> worker_info;
    size_t worker_num;
  public:
    CoordinatorServer(const size_t worker_size,
                      const std::function<void(const cookie_set &)> &send_cookie);
    void firstconnection(const int conn_id,
                         const message::ReadOnlyBuffer &buf);
    void broadcast(message::ZeroCopyBuffer &&buf);
    void peersinfo(const int, const message::ReadOnlyBuffer &);
};

class WorkerDeamon;
using WkDeamonBase = StateMachine<WorkerDeamon, ClientModel>;
class WorkerDeamon : public WkDeamonBase {
  private:
    FirstConCookie fc_cookie;
    WorkerInfo worker_info;

  public:
    WorkerDeamon(const std::string &cookie, const WorkerInfo & info);
    void init_logic();
    void firstconnection_ok(const int conn_id,
                            const message::ReadOnlyBuffer &buf);
    void recv_worker_info(const int conn_id,
                            const message::ReadOnlyBuffer &buf);
};

class WorkerDataServer;
using WorkerDataServerBase = StateMachine<WorkerDataServer, ServerModel>;
class WorkerDataServer: public WorkerDataServerBase{
    private:
    public:
        WorkerDataServer();
        WorkerDataServer(const WorkerDataServer &other) = delete;
        WorkerDataServer(WorkerDeamon &&) = delete;
};

host_file_entry parse_host_file_entry(const std::string &);

std::vector<host_file_entry> read_host_file(const char *);


_THALLIUM_END_NAMESPACE

#endif
