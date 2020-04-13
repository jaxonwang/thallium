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
    static_assert(is_nothrow_copy_assignable<T>::value ||
                      is_nothrow_move_assignable<T>::value,
                  "The elements should be nothrow copy assignable or nothrow "
                  "move assignable!");

  protected:
    std::queue<T> msgq;
    mutable std::mutex q_mux;  // TODO: performance optimization
    std::condition_variable has_msg_cd;

  public:
    Channel() {}
    Channel(const Channel &) = delete;
    Channel(Channel &&) = delete;
    ~Channel() {}
    template <class T_, class = typename enable_if<
                            is_same<typename decay<T_>::type, T>::value>::type>
    void send(T_ &&t) {
        {
            std::lock_guard<std::mutex> lock{q_mux};
            msgq.push(forward<T_>(t));
        }
        has_msg_cd.notify_one();
    }
    void receive(T &t) {
        std::unique_lock<std::mutex> lk(q_mux);
        has_msg_cd.wait(lk, [&] { return !msgq.empty(); });
        t = msgq.front();
        msgq.pop();
    }
    template <class Rep, class Period>
    bool receive_for(T &t, const std::chrono::duration<Rep, Period> &rel_time) {
        std::unique_lock<std::mutex> lk(q_mux);
        bool success =
            has_msg_cd.wait_for(lk, rel_time, [&] { return !msgq.empty(); });
        if (success) {
            t = msgq.front();
            msgq.pop();
        }
        return success;
    }
};

_THALLIUM_END_NAMESPACE

#endif
