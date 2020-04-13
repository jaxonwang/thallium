#ifndef _THALLIUM_CONCURRENT
#define _THALLIUM_CONCURRENT

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <type_traits>

#include "common.hpp"
using namespace std;

_THALLIUM_BEGIN_NAMESPACE

template <class T>
class Channel {
    static_assert(is_copy_assignable<T>::value,
                  "The elements should be copy assignable!");

  protected:
    std::queue<T> msgq;
    mutable std::mutex q_mux;  // TODO: performance optimization
    std::condition_variable has_msg_cd;

  public:
    Channel() {}
    Channel(const Channel &) = delete;
    Channel(Channel &&) = delete;
    ~Channel() {}
    template <class T_, class = typename enable_if<is_same<T_, T>::value>::type>
    void send(T_ &&t) {
        {
            std::lock_guard<std::mutex> lock{q_mux};
            msgq.push_back(forward<T_>(t));
        }
        has_msg_cd.notify_one();
    }
    void receive(T &t) {
        std::unique_lock<std::mutex> lk(q_mux);
        has_msg_cd.wait(lk, [&] { return !msgq.empty(); });
        t = msgq.top();
        msgq.pop();
    }
};

_THALLIUM_END_NAMESPACE

#endif
