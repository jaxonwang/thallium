#include "thallium.hpp"

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

_THALLIUM_END_NAMESPACE
