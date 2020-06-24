#include "thallium.hpp"

#include <cstdlib>

#include "context.hpp"
#include "network.hpp"

using namespace std;

_THALLIUM_BEGIN_NAMESPACE

Execution::Execution(const Place &place, const FuncId &f_id, BuffersPtr &&s_l)
    : id(current_id++),
      f_id(f_id),
      place(place),
      s_l(move(s_l)),
      state{ExeState::pre_run} {}

Execution::ExeId Execution::current_id;

Execution::ExeId ExecutionHub::newExecution(const Place &place,
                                            const FuncId &f_id,
                                            BuffersPtr &&s_l) {
    // note: can be called from different thread
    unique_ptr<Execution> p{new Execution(place, f_id, move(s_l))};
    auto id = p->id;
    all_exes.insert({id, move(p)});
    // TODO add to pending
    return id;
}

Buffer ExecutionHub::wait(Execution::ExeId id) {  // TODO FAKE wait for
    // note: can be called from different thread
    auto &pre_e = all_exes[id];
    // TODO relase the execution object if called
    return Buffer{};
}

// factory method
Execution::ExeId ExecutionFactory(const Place &place, const FuncId &f_id,
                                  BuffersPtr &&s_l) {
    return ExecutionHub::get()->newExecution(place, f_id, move(s_l));
}

void FinishMonitor::waitAll() {
    for (auto &x : exes) {
        wait(x);
    }
}

void FinishStack::push(unique_ptr<FinishMonitor> &&fm_ptr) {
    f_stack.push_back(move(fm_ptr));
}

unique_ptr<FinishMonitor> &FinishStack::get_top() {
    if (f_stack.empty()) {
        auto e = std::logic_error(
            "There is no finish in finish stack. Should not reach here!");
        TI_RAISE(e);
    }
    return f_stack.back();
}

void FinishStack::delete_top() { f_stack.pop_back(); }

void FinishStack::newFinish() {
    push(unique_ptr<FinishMonitor>{new FinishMonitor{}});
}

void FinishStack::endFinish() {
    get_top()->waitAll();
    delete_top();
}

void FinishStack::addExecutionToCurrentFinish(Execution::ExeId &id) {
    get_top()->addExecution(id);
}

Execution::ExeId AsyncExecManager::submitExecution(const Place &place,
                                                   const FuncId &f_id,
                                                   BuffersPtr &&s_l) {
    auto id = ExecutionFactory(place, f_id, move(s_l));
    // TODO turn prerun into running: release the buffers' mem
    // TODO submit exe to run
    FinishStack::get()->addExecutionToCurrentFinish(id);
    return id;
}

Execution::ExeId BlockedExecManager::submitExecution(const Place &place,
                                                     const FuncId &f_id,
                                                     BuffersPtr &&s_l) {
    auto id = ExecutionFactory(place, f_id, move(s_l));
    return id;
}

void init_statics() {
    BlockedExecManager::get();
    Execution::init_statics();
}

void stderr_and_quit(const char *msg) {
    cerr << msg << endl;
    exit(-1);
}

void stderr_and_quit(const string &msg) {
    cerr << msg << endl;
    exit(-1);
}

void thallium_init() {
    // start server
    Context ctx;
    const char *env_coord_host = getenv(ENVNAME_COORD_HOST);
    const char *env_coord_port = getenv(ENVNAME_COORD_PORT);
    if (!env_coord_host) {
        stderr_and_quit(
            format("Environment varialble not set: {}", ENVNAME_COORD_HOST));
    }
    if (!valid_hostname(env_coord_host)) {
        stderr_and_quit(format("Invalid coordinator hostname: {} = {}",
                               ENVNAME_COORD_HOST, env_coord_host));
    }
    ctx.coordinator_host = env_coord_host;
    if (!env_coord_port) {
        stderr_and_quit(
            format("Environment varialble not set: {}", ENVNAME_COORD_PORT));
    }

    if (-1 == (ctx.coordinator_port = port_to_int(env_coord_host))) {
        stderr_and_quit(format("Invalid coordinator port: {} = {}",
                               ENVNAME_COORD_PORT, env_coord_port));
    }
}

_THALLIUM_END_NAMESPACE
