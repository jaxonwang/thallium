#include <chrono>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "exception.hpp"
#include "serialize.hpp"
#include "utils.hpp"

_THALLIUM_BEGIN_NAMESPACE
using namespace std;

struct closure_holder {};

struct finish {
    static void start();
    static void end();
};

class Place {};  // TODO: finish here

static Place self{};

class Execution {
    static int current_id;  // TODO: thread safe
   public:
    const int id;
    const string f_id;
    const Place place;
    static void init_statics() { current_id = 0; }
    Execution(const Place &place, const string &f_id)
        : id(current_id++), f_id(f_id), place(place) {}
    Execution() = delete;
};
int Execution::current_id;

class PreRunExecution : public Execution {
   public:
    const BuffersPtr s_l;
    PreRunExecution(const Place &place, const string &f_id, BuffersPtr &&s_l)
        : Execution(place, f_id), s_l(move(s_l)) {}
};

class RunningExecution : public Execution {
   public:
    RunningExecution();
};

class AsyncRemoteExecution : public RunningExecution {};

class AsyncLocalExecution : public RunningExecution {};

class ThreadExecutionHandler {};

class Submitter {};

class ExecManager {};
class AsyncExecManager : public ExecManager {};  // TODO manager for finish
class BlockedExecManager : public ExecManager,
                           public Singleton<BlockedExecManager> {  // singleton
    friend class Singleton<BlockedExecManager>;

   private:
    typedef std::unique_ptr<Execution> ExecutionPtr;
    typedef std::shared_ptr<PreRunExecution> PreRunExecutionPtr;
    std::queue<PreRunExecutionPtr>
        execution_unprocessed;  // TODO: thread safe queue
    std::unordered_map<int, ExecutionPtr> running_exectuion;

   public:
    int submitExecution(const Place &place, const string &f_id,
                        BuffersPtr &&s_l) {
        PreRunExecutionPtr p{new PreRunExecution(place, f_id, move(s_l))};
        int id = p->id;
        execution_unprocessed.push(move(p));
        return id;
    }
    void pickToRun() {}
    string waitFor(int id) {  // TODO FAKE wait for
        while (true) {
            if (execution_unprocessed.size() == 0) {
                std::this_thread::sleep_for(chrono::seconds(2));
                continue;
            } else {
                auto pre_e = move(execution_unprocessed.front());
                execution_unprocessed.pop();
                return (*pre_e->s_l)[0];
                // TODO conver to running and add finised queue
            }
        }
    }
};

// also doing serializtion
class Coordinator {
   protected:
    Coordinator(){};

   public:
    // blocked submit
    template <class Ret, class... ArgTypes>
    static int BlockedSubmit(Place &place, Ret(f)(ArgTypes...),
                             ArgTypes... args) {  // TODO args copied?
        const string f_id = function_id(f);
        // TODO: no serialization needed if run local
        // TODO: if local submit to local execution schedualler
        auto s_l = Serializer::serializeList(args...);
        int id =
            BlockedExecManager::get()->submitExecution(place, f_id, move(s_l));
        return id;
    }
    template <class Ret>
    static Ret getReturnValue(int e_id) {
        string ret = BlockedExecManager::get()->waitFor(e_id);
        return Serializer::deSerialize<Ret>(ret);
    }
    // unblocked submit
    template <class Ret, class... ArgTypes>
    static void Submit(Place &place, Ret(f)(ArgTypes...),
                       ArgTypes... args) {  // TODO args copied?
        std::string f_id = function_id(f);
        // TODO: no serialization needed if run local
        // TODO: if local submit to local execution schedualler
        auto s_l = Serializer::serializeList(
            args...);  // TODO serialize in different thread
                       // Submit to current finish layer manager
    }
};

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

void init_statics() {
    BlockedExecManager::get();
    Execution::init_statics();
}

_THALLIUM_END_NAMESPACE

// user privede ti_main as main entry
// int ti_main(int argc, const char *argv[]) {
//     do something
//     return 0;
// }

#define TI_MAIN()\
int main(int argc, const char *argv[]) {\
    thallium::init_statics();\
    int ret;\
    try {\
        ret = ti_main(argc, argv);\
    } catch (const std::exception &e) {\
        thallium::ti_exception::handle_uncatched_and_abort(e);\
    }\
    \
    return ret;                                                \
    }

