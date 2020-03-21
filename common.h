
#ifndef _THALLIUM_COMMON
#define _THALLIUM_COMMON

#define _THALLIUM_BEGIN_NAMESPACE namespace thallium {
#define _THALLIUM_END_NAMESPACE }


#include<typeinfo>
#include<string>

_THALLIUM_BEGIN_NAMESPACE
template <class Rt, class ... Argtypes>
const std::string function_id(const Rt(f)(Argtypes...)){
  return typeid(f).name();
}
_THALLIUM_END_NAMESPACE

#endif
