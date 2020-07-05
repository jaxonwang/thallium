#ifndef _THALLIUM_CONCURRENT
#define _THALLIUM_CONCURRENT

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stack>
#include <thread>
#include <type_traits>

#include "common.hpp"

_THALLIUM_BEGIN_NAMESPACE


// will always try to move if possible
template <class T, typename std::enable_if<is_nothrow_move_assignable<T>::value,
                                           int>::type = 0>
inline void move_or_copy(T &to, T &from) {
    to = move(from);
}

template <class T,
          typename std::enable_if<is_nothrow_copy_assignable<T>::value &&
                                      !is_nothrow_move_assignable<T>::value,
                                  int>::type = 0>
inline void move_or_copy(T &to, T &from) {
    to = from;
}

template <class T>
// T should not be shared/raw pointer, otherwise same object can be refered in
// both side
class BasicLockQueue {  // multi producers multi consumers
    static_assert(is_nothrow_copy_assignable<T>::value ||
                      is_nothrow_move_assignable<T>::value,
                  "The elements should be nothrow copy assignable or nothrow "
                  "move assignable!");

  protected:
    std::queue<T> msgq;
    mutable std::mutex q_mux;
    std::condition_variable has_msg_cd;

  public:
    BasicLockQueue() {}
    BasicLockQueue(const BasicLockQueue &) = delete;
    BasicLockQueue(BasicLockQueue &&) = delete;
    ~BasicLockQueue() {}
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
        // if you don't like copy, use unique ptr for T
        std::unique_lock<std::mutex> lk(q_mux);
        has_msg_cd.wait(lk, [&] { return !msgq.empty(); });
        move_or_copy(t, msgq.front());
        msgq.pop();
    }

    bool try_receive(T &t){
        std::lock_guard<std::mutex> lock{q_mux};
        if(msgq.empty())
            return false;
        move_or_copy(t, msgq.front());
        msgq.pop();
    }

    template <class Rep, class Period>
    bool receive_for(T &t, const std::chrono::duration<Rep, Period> &rel_time) {
        std::unique_lock<std::mutex> lk(q_mux);
        bool success =
            has_msg_cd.wait_for(lk, rel_time, [&] { return !msgq.empty(); });
        if (success) {
            move_or_copy(t, msgq.front());
            msgq.pop();
        }
        return success;
    }
};

template <class T, int buffer_size = 4096>  // single producer consumer
class LockFreeChannel {                     // lamport queue
  private:
    T ring_buffer[buffer_size];
    std::atomic_size_t head;
    std::atomic_size_t tail;
    size_t inline next(size_t pos) { return (pos + 1) % buffer_size; }

  public:
    LockFreeChannel() : head(0), tail(0) {}
    LockFreeChannel(const LockFreeChannel &c) = delete;
    LockFreeChannel(LockFreeChannel &&c) = delete;

    template <class T_, class = typename enable_if<
                            is_same<typename decay<T_>::type, T>::value>::type>
    bool try_send(T_ &&t) {  // copy only
        auto head_v = head.load(std::memory_order_relaxed);
        auto tail_v = tail.load(std::memory_order_acquire);
        if (next(head_v) == tail_v) {
            return false;
        }
        ring_buffer[head_v] = std::forward<T_>(t);
        head.store(next(head_v), std::memory_order_release);
        return true;
    }

    bool try_receive(T &t) {
        auto head_v = head.load(std::memory_order_relaxed);
        auto tail_v = tail.load(std::memory_order_acquire);
        if (head_v == tail_v) {
            return false;
        }
        move_or_copy(t, ring_buffer[tail_v]);
        tail.store(next(tail_v), std::memory_order_release);
        return true;
    }

    template <class T_, class = typename enable_if<
                            is_same<typename decay<T_>::type, T>::value>::type>
    void send(T_ &&data) {  // will block
        for (int t = 2; !try_send(std::forward<T_>(data));) {
            std::this_thread::sleep_for(std::chrono::microseconds(t));
            if (t < 1024) t *= 2;
        }
    }

    void receive(T &data) {  // will block
        for (int t = 2; !try_receive(data);) {
            std::this_thread::sleep_for(std::chrono::microseconds(t));
            if (t < 1024) t *= 2;
        }
    }
};

template <class T>
// T should not be shared/raw pointer, otherwise same object can be
// refered in both side
class SenderSideLockQueue {  // senario: logging, one active
                             // consumer, many less frequent producers
                             // infinite size
    static_assert(is_nothrow_copy_assignable<T>::value ||
                      is_nothrow_move_assignable<T>::value,
                  "The elements should be nothrow copy assignable or nothrow "
                  "move assignable!");

  protected:
    std::queue<T> in_queue;
    std::queue<T> out_queue;
    mutable std::mutex in_lock;
    std::condition_variable has_msg_cd;

  public:
    SenderSideLockQueue() {}
    SenderSideLockQueue(const SenderSideLockQueue &) = delete;
    SenderSideLockQueue(SenderSideLockQueue &&) = delete;
    ~SenderSideLockQueue() {}
    template <class T_, class = typename enable_if<
                            is_same<typename decay<T_>::type, T>::value>::type>
    void send(T_ &&t) {
        // non block
        {
            std::lock_guard<std::mutex> lock{in_lock};
            in_queue.push(forward<T_>(t));
        }
        has_msg_cd.notify_one();
    }
    void receive(T &t) {
        while (true) {
            if (out_queue.size() > 0) {
                move_or_copy(t, out_queue.front());
                out_queue.pop();
                return;
            } else {
                std::unique_lock<std::mutex> iq{in_lock};
                has_msg_cd.wait(iq, [&] { return !in_queue.empty(); });
                out_queue.swap(in_queue);
            }
        }
    }

    bool try_receive(T &t){
        if(out_queue.size() == 0)
            return false;
        move_or_copy(t, out_queue.front());
        out_queue.pop();
        return true;
    }

    template <class Rep, class Period>
    bool receive_for(T &t, const std::chrono::duration<Rep, Period> &rel_time) {
        while (true) {
            if (out_queue.size() > 0) {
                move_or_copy(t, out_queue.front());
                out_queue.pop();
                return true;
            } else {
                std::unique_lock<std::mutex> iq{in_lock};
                bool success = has_msg_cd.wait_for(
                    iq, rel_time, [&] { return !in_queue.empty(); });
                if (success)
                    out_queue.swap(in_queue);
                else
                    return false;
            }
        }
    }
};

_THALLIUM_END_NAMESPACE

#endif
