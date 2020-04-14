#include "function_object.hpp"

using namespace std;

_THALLIUM_BEGIN_NAMESPACE

FuncId FuncIdGen::current = 0;
unordered_map<string, FuncId> FuncIdGen::id_lookup{};

FuncId FuncIdGen::getIdfromMangled(const string &s) {
    if (id_lookup.count(s)) {
        return id_lookup[s];
    }
    auto i = current++;
    id_lookup.insert({s, i});
    return i;
}

void FuncManager::addFunc(const FuncId &f_id,
                          const shared_ptr<FunctionObjectBase> &f_ptr) {
    if (f_table.count(f_id) != 0)
        TI_RAISE(std::logic_error(
            thallium::format("The function: {} has been registered!", f_id)));
    f_table.insert({f_id, f_ptr});
}

shared_ptr<FunctionObjectBase> FuncManager::getFuncObj(const FuncId &f_id) {
    return f_table[f_id];
}

shared_ptr<FunctionObjectBase> get_function_object(const FuncId &f_id) {
    return FuncManager::get()->getFuncObj(f_id);
}

_THALLIUM_END_NAMESPACE
