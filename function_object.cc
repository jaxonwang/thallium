#include <memory>
#include <iostream>
#include <boost/any.hpp>
#include <vector>

#include "common.hpp"
#include "serialize.hpp"

_THALLIUM_BEGIN_NAMESPACE

using namespace std;

using BoxValue=boost::any;
class FunctionObject{
protected:
  virtual void doCall()=0;
public:
  template <class Ret, class ...ArgTypes>
    Ret operator()(ArgTypes...args){
    }

};

template <class Ret, class ...ArgTypes>
class FunctionObjectImpl:public FunctionObject{
};

template <class Ret, class ...ArgTypes>
class FunctionSingnature{ 
  vecor<BoxValue> deSerializeArguments(const vector<string> &buffers){
    auto args = vector<BoxValue>{};
   for(auto &arg: buffers){
    args.push_back(move(deSerialize(arg))) 
   }
  }
};




_THALLIUM_END_NAMESPACE

#include <typeinfo>

void test1(int f(int)){
  std::cout << typeid(f).hash_code() <<std::endl;
}

void test2(int (*f)(int)){
  std::cout << typeid(f).hash_code() <<std::endl;
}

int main(){

  auto p = [](int a){return a+1;};

  test1(p);
  test2(p);

}
