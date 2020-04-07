#ifndef _THALLIUM_UTILS
#define _THALLIUM_UTILS

#include <functional>
#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE

template <class C>
class Singleton {
   private:
    static C *_instance;

   protected:
    Singleton(){}
   public:
    static void init() { Singleton::_instance = new C{}; }
    static C *get() {
        if (!_instance)  // TODO: thread safe or init manually
            init();
        return _instance;
    }
    Singleton(const Singleton &&) = delete; // disable copy for all derived classes
    Singleton & operator=(const Singleton &&) = delete;
};
template <class C>
C *Singleton<C>::_instance = nullptr;

_THALLIUM_END_NAMESPACE

#endif

