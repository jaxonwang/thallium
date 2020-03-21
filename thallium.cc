#include <iostream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <string>

#include "common.h"

_THALLIUM_BEGIN_NAMESPACE
using namespace std;
using SerializedList=unique_ptr<vector<string>>;

struct closure_holder {};

struct finish {
  static void start();
  static void end();
};

class Execution {
public:
  const int id;
  const string f_id;
  Execution(int id, string f_id):id(id),f_id(f_id){}
  Execution() = delete;
};

class PreRunExecution:Execution{
public:
  const SerializedList s_l;
  PreRunExecution();
};

class AsyncExecution : Execution {
public:
  AsyncExecution();
};

class AsyncRemoteExecution : public AsyncExecution {};

class AsyncLocalExecution : public AsyncExecution {};

class ThreadExecutionHandler {};

class Place {};

static Place self{};

class Submitter {};

class AsyncSubmitter : Submitter {
  Place place;
  AsyncSubmitter() {}
  AsyncSubmitter(const Place &p) : place(p) {}
  template <class Ret, class... ArgTypes>
  void operator()(Ret(f)(ArgTypes...), ArgTypes... args){};
};

class BlockedSubmitter : Submitter {
  Place place;
  BlockedSubmitter() {}
  BlockedSubmitter(const Place &p) : place(p) {}
  template <class Ret, class... ArgTypes>
  Ret operator()(Ret(f)(ArgTypes...), ArgTypes... args){};
};

template <typename T>
string my_serialized_method(T &a){ //TODO: user implement serialization api
  return to_string(a);
}

string my_serialized_method(string &a){
  return a;
}


class Serializer{
  private:
  static SerializedList _serialize(SerializedList && s_l){
    return move(s_l);
  }
  template <class T, class... ArgTypes>
  static SerializedList _serialize(SerializedList && s_l, T t, ArgTypes... args){
    s_l->push_back(my_serialized_method(t));
    return _serialize(move(s_l), args...); 
  }
  public:
  template <class Ret, class... ArgTypes>
  static SerializedList serialize(ArgTypes... args){
    SerializedList s_l{new vector<string>};
    return move(_serialize(s_l, args...));
  }
};

// also doing serializtion
class Coordinator { // TODO: make singleton code reuseable
private:
  static Coordinator *_instance;
  typedef std::unique_ptr<Execution> ExecutionPtr;
  std::queue<Execution> un_dealed_execution; // TODO: thread safe queue
  std::unordered_map<int, Execution> running_exectuion;

protected:
  Coordinator(){};

public:
  template <class Ret, class... ArgTypes>
  void Submit(Ret(f)(ArgTypes...), ArgTypes... args) {
    std::string f_id = function_id(f);
    auto s_l = Serializer::serialize(args...); //TODO serialize in different thread
  

  }

  static Coordinator *get() {
    if (!_instance) // TODO: thread safe or init manually
      _init();
    return _instance;
  }

  static void _init() { Coordinator::_instance = new Coordinator{}; }
};

_THALLIUM_END_NAMESPACE

int main(int argc, const char *argv[]) {
  std::cout << "hello" << std::endl;

  return 0;
}
