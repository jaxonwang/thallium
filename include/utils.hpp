#ifndef _THALLIUM_UTILS
#define _THALLIUM_UTILS

#include <mutex>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

template <class C>
class Singleton { // this signleton can not be recreated once init.
  protected:
    Singleton() {}

  public:
    static C &get() {
        static C _instance;
        return _instance;
    }
    Singleton(const Singleton &) =
        delete;  // disable copy for all derived classes
    Singleton(Singleton &&) = delete;
    Singleton &operator=(const Singleton &) = delete;
    Singleton &operator=(Singleton &&) = delete;
};

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
