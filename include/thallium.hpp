#ifndef _THALLIUM_HEADER
#define _THALLIUM_HEADER
#include <chrono>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "exception.hpp"
#include "function_object.hpp"
#include "serialize.hpp"
#include "utils.hpp"

using namespace std;

_THALLIUM_BEGIN_NAMESPACE

struct closure_holder {};

struct finish {
    static void start();
    static void end();
};

class Place {};  // TODO: finish here

static Place this_host{};

enum class ExeState { pre_run, disptached, running, collecting, finished };

class ExecutionHub;
class Execution {  // TODO: state machine might be better
    friend class ExecutionHub;

  public:
    typedef unsigned int ExeId;
    static void init_statics() { current_id = 0; }

  protected:
    const ExeId id;
    const FuncId f_id;
    const Place place;
    const BuffersPtr s_l;
    ExeState state;
    Execution(const Place &place, const FuncId &f_id, BuffersPtr &&s_l);
    Execution() = delete;
    static ExeId current_id;  // TODO: thread safe mark it atomic?
};

class ExecutionHub : public Singleton<ExecutionHub> {
    friend class Singleton<ExecutionHub>;
    typedef std::unique_ptr<Execution> ExecutionPtr;

  private:
    std::unordered_map<Execution::ExeId, ExecutionPtr> all_exes;
    std::queue<Execution::ExeId> pending_exes;  // TODO: thread safe queue
  public:
    Execution::ExeId newExecution(const Place &, const FuncId &, BuffersPtr &&);
    Buffer wait(Execution::ExeId id);
};

class ExecutionWaitor {  // wait interface
  public:
    Buffer wait(Execution::ExeId id) { return ExecutionHub::get().wait(id); }
};

class FinishStack;
class FinishMonitor : public ExecutionWaitor {
    friend class FinishStack;
    vector<Execution::ExeId> exes;  // TODO thread safe
  public:
    FinishMonitor() {}
    void waitAll();
    void addExecution(Execution::ExeId &id) { exes.push_back(id); }
};

class FinishStack : public Singleton<FinishStack> {
    friend class Singleton<FinishStack>;
    list<unique_ptr<FinishMonitor>> f_stack;  // TODO thread safe
  public:
    void push(unique_ptr<FinishMonitor> &&fm_ptr);
    void delete_top();
    void newFinish();
    void endFinish();
    void start() { newFinish(); }
    void end() { endFinish(); }
    void addExecutionToCurrentFinish(Execution::ExeId &id);
    unique_ptr<FinishMonitor> &get_top();
};

class FinishSugar {
  public:
    bool once;
    FinishSugar() : once(true) { FinishStack::get().newFinish(); }
    ~FinishSugar() { FinishStack::get().endFinish(); }
};

class ThreadExecutionHandler {};

class ExecManager {};
class AsyncExecManager : public ExecManager,
                         public Singleton<AsyncExecManager> {
    friend class Singleton<AsyncExecManager>;

  public:
    Execution::ExeId submitExecution(const Place &, const FuncId &,
                                     BuffersPtr &&);
};

class BlockedExecManager : public ExecManager,
                           public ExecutionWaitor,
                           public Singleton<BlockedExecManager> {  // singleton
    friend class Singleton<BlockedExecManager>;

  public:
    Execution::ExeId submitExecution(const Place &, const FuncId &,
                                     BuffersPtr &&);
};

// direct call from user, encapsulate the exe submission
// also doing serializtion
class Coordinator {
  protected:
    Coordinator(){};

    template <class MngT, class Ret, class... ArgTypes>
    static Execution::ExeId RemoteSubmit(
        Place &place, Ret(f)(ArgTypes...),
        const ArgTypes &... args) {  // TODO args copied? now let forbid the &&
        const FuncId f_id = function_id(f);
        auto s_l = Serializer::serializeList(args...);
        Execution::ExeId id =
            MngT::get().submitExecution(place, f_id, move(s_l));
        return id;
    }

  public:
    // blocked submit
    template <class Ret, class... ArgTypes>
    static Execution::ExeId BlockedSubmit(
        Place &place, Ret(f)(ArgTypes...),
        const ArgTypes &... args) {  // TODO args copied? now let forbid the &&
        // TODO: no serialization needed if run local
        // TODO: if local submit to local execution schedualler
        Execution::ExeId id;
        if (false) {  // if islocal

        } else {
            // Serialization in Blocked submit can be blocked
            id = RemoteSubmit<BlockedExecManager>(place, f, args...);
        }
        return id;
    }
    template <class Ret>
    static Ret getReturnValue(const Execution::ExeId e_id) {
        Buffer ret = BlockedExecManager::get().wait(e_id);
        return Serializer::create_from_string<Ret>(ret);
    }

    // unblocked submit
    template <class Ret, class... ArgTypes>
    static void Submit(Place &place, Ret(f)(ArgTypes...),
                       const ArgTypes &... args) {  // TODO args copied?
        // TODO: no serialization needed if run local
        // TODO: if local submit to local execution schedualler
        if (false) {  // if is local
        } else {
            // TODO: new tread and to below
            RemoteSubmit<AsyncExecManager>(place, f, args...);
        }
    }
};

class Submitter {};

class AsyncSubmitter : public Submitter {
    Place place;
    AsyncSubmitter() {}
    AsyncSubmitter(const Place &p) : place(p) {}
    template <class Ret, class... ArgTypes>
    void operator()(Ret(f)(ArgTypes...), ArgTypes... args) {}
};

class BlockedSubmitter : public Submitter {
    Place place;

  public:
    BlockedSubmitter() {}
    BlockedSubmitter(const Place &p) : place(p) {}
    template <class Ret, class... ArgTypes>
    Ret operator()(Ret(f)(ArgTypes...), ArgTypes... args) {
        auto id = Coordinator::BlockedSubmit(place, f, args...);
        return Coordinator::getReturnValue<Ret>(id);
    }
};

void init_statics();

_THALLIUM_END_NAMESPACE

// user privede ti_main as main entry
// int ti_main(int argc, const char *argv[]) {
//     do something
//     return 0;
// }

void thallium_init();

#define TI_MAIN()                                                  \
    int main(int argc, const char *argv[]) {                       \
        thallium::init_statics();                                  \
        int ret;                                                   \
        try {                                                      \
            ret = ti_main(argc, argv);                             \
        } catch (const std::exception &e) {                        \
            thallium::ti_exception::handle_uncatched_and_abort(e); \
        }                                                          \
                                                                   \
        return ret;                                                \
    }

#endif
