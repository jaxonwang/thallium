#ifndef _THALLIUM_UTILS
#define _THALLIUM_UTILS

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

template< class T >
struct remove_cvref {
    typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

template< class T >
using remove_cvref_t = typename remove_cvref<T>::type;

_THALLIUM_END_NAMESPACE

#endif

