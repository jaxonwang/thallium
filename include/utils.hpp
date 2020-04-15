#ifndef _THALLIUM_UTILS
#define _THALLIUM_UTILS

#include <mutex>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

template <class C>
class Singleton {
  private:
    static C *_instance;
    static std::once_flag once;

  protected:
    Singleton() {}

  public:
    static void init() { Singleton::_instance = new C{}; }
    static C *get() {
        if(!_instance){
            std::call_once(once, init);
        }
        return _instance;
    }
    Singleton(const Singleton &) =
        delete;  // disable copy for all derived classes
    Singleton(Singleton &&) = delete;
    Singleton &operator=(const Singleton &) = delete;
    Singleton &operator=(Singleton &&) = delete;
};
template <class C>
C *Singleton<C>::_instance = nullptr;

template <class C>
std::once_flag Singleton<C>::once;

template <class T>
struct remove_cvref {
    typedef
        typename std::remove_cv<typename std::remove_reference<T>::type>::type
            type;
};

template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;

_THALLIUM_END_NAMESPACE

#endif
