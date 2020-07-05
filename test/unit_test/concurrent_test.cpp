#include "concurrent.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "test.hpp"

using namespace thallium;
using namespace std;

template <class T>
class LockFreeChannel4096 : public LockFreeChannel<T, 4096> {};

template <template <class> class C>
void run_test_order() {
    const int arr_size = 7;
    int a[arr_size] = {7, 6, 5, 4, 3, 2, 1};

    C<int> c;
    for (auto &e : a) {
        c.send(e);
    }
    for (int i = 0; i < arr_size; i++) {
        int r;
        c.receive(r);
        ASSERT_EQ(a[i], r);
    }

    C<string> c1;
    vector<string> str_v{"1dsaf", "harsdg", "hello", "yktrsl", "asdfppp"};
    auto str_v1 = str_v;
    for (auto &s : str_v) {
        c1.send(s);
    }
    int c1_size = str_v.size();
    for (int i = 0; i < c1_size; i++) {
        string a;
        c1.receive(a);
        ASSERT_EQ(a, str_v[i]);
    }

    for (auto &s : str_v) {
        c1.send(move(s));
    }

    for (int i = 0; i < c1_size; i++) {
        string a;
        c1.receive(a);
        ASSERT_EQ(a, str_v1[i]);
        ASSERT_EQ("", str_v[i]);
    }

    C<unique_ptr<int>> ptr_c;
    for (int i = 0; i < 10; i++) {
        ptr_c.send(unique_ptr<int>{new int(i)});
    }
    for (int i = 0; i < 10; i++) {
        unique_ptr<int> tmp;
        ptr_c.receive(tmp);
        ASSERT_EQ(*tmp, i);
    }
}
TEST(ChannelTest, TestOrder) {
    run_test_order<BasicLockQueue>();
    run_test_order<SenderSideLockQueue>();
    run_test_order<LockFreeChannel4096>();
}

template <template <class> class C>
void run_test_concurrent_order() {
    // test concurrent order
    //
    C<int> c;
    const int vec_size = 128;
    const int thread_num = 10;
    vector<thread> ts(thread_num);
    vector<int> max_receved(
        thread_num, -1);  // to ensure order, the max received from ith thread;
    for (int i = 0; i < thread_num; i++) {
        ts.push_back(thread{[&c, i]() {
            for (int j = 0; j < vec_size; j++) {
                c.send(i * 128 + j);
            }
        }});
    }
    for (int i = 0; i < vec_size * thread_num; i++) {
        int r;
        c.receive(r);
        int k = r / 128;
        int v = r % 128;
        ASSERT_TRUE(max_receved[k] < v);
        max_receved[k] = v;
    }
    for (int i = 0; i < thread_num; i++) {
        ts[i].join();
    }
}

TEST(ChannelTest, ConcurrentOrder) {
    run_test_order<BasicLockQueue>();
    run_test_order<SenderSideLockQueue>();
}

TEST(ChannelTest, SingleChannelConcurrentOrder) {
    LockFreeChannel4096<int> c;
    const int arr_size = 128;
    thread t{[&c]() {
        for (int j = 0; j < arr_size; j++) {
            c.send(j);
        }
    }};
    int min_received = -1;
    for (int i = 0; i < arr_size; i++) {
        int r;
        c.receive(r);
        ASSERT_TRUE(min_received < r);
        min_received = r;
    }
    t.join();
}

template <template <class> class C>
void run_try_receive() {
    const int arr_size = 7;
    int a[arr_size] = {7, 6, 5, 4, 3, 2, 1};

    C<int> c;
    for (auto &e : a) {
        c.send(e);
    }

    vector<thread> ts(arr_size);
    for (int i = 0; i < arr_size; i++) {
        ts.push_back(thread{[&c, i]() { c.send(i); }});
    }
    for (int i = 0; i < arr_size; i++) {
        int r;
        ASSERT_TRUE(c.try_receive(r));
    }
    for (int i = 0; i < arr_size; i++) {
        ts[i].join();
    }
    for (int i = 0; i < 7; i++) {
        int r;
        ASSERT_FALSE(c.try_receive(r));
    }
    // send again to see try_receive will success;
    for (auto &e : a) {
        c.send(e);
    }
    for (int i = 0; i < arr_size; i++) {
        int r;
        ASSERT_TRUE(c.try_receive(r));
    }
}

template <template <class> class C>
void run_test_recevie_for() {
    C<string> c;
    C<string> c1;
    chrono::milliseconds wait_time{1};
    int t1_ret = 1234;
    thread t1{[&]() {
        string tmp;
        bool r = c.receive_for(tmp, wait_time);
        if (!r) t1_ret = 4321;
    }};

    string t2_s;
    thread t2{[&]() {
        string tmp;
        bool r = c1.receive_for(tmp, chrono::milliseconds{500});
        if (r) t2_s = tmp;
    }};
    // only send to c1
    thread t3{[&]() { c1.send(string{"just test"}); }};

    t1.join();
    t2.join();
    t3.join();
    ASSERT_EQ(t1_ret, 4321);
    ASSERT_EQ(t2_s, "just test");
}

TEST(ChannelTest, ReceiveFor) {
    run_test_recevie_for<BasicLockQueue>();
    run_test_recevie_for<SenderSideLockQueue>();
}
template <template <class> class C>
void run_test_concurrent_single_rw() {
    auto sum = [&](int n) -> int {  // not correct just for test
        C<int> c;
        long long ret = 0;
        thread t1{[=, &c] {
            for (int j = 0; j < n; j++) {
                c.send(j);
            }
        }};
        thread t_sum{[&] {
            long long s = 0;
            for (int i = 0; i < n; i++) {
                int tmp;
                c.receive(tmp);
                s += tmp;
            }
            ret = s;
        }};
        t1.join();
        t_sum.join();
        return ret;
    };

    ASSERT_EQ(sum(100000), 704982704);
}

template <template <class> class C>
void run_test_concurrent_multi_write() {
    // test concurrent access
    auto sum = [&](int n) -> int {  // not correct just for test
        n++;
        C<int> c;
        int ret = 0;
        vector<thread> ts;
        for (int i = 0; i < n; i++) {
            ts.push_back(thread{[=, &c] {
                this_thread::sleep_for(chrono::milliseconds(10));
                for (int j = 0; j < 100; j++) {
                    c.send(i);
                }
            }});
        }
        thread t_sum{[&] {
            long long s = 0;
            for (int i = 0; i < n * 100; i++) {
                int tmp;
                c.receive(tmp);
                s += tmp;
            }
            ret = s;
        }};
        t_sum.join();
        for (auto &t : ts) {
            t.join();
        }
        return ret;
    };

    ASSERT_EQ(sum(100), 505000);
}

TEST(ChannelTest, BaseLine) {
    BasicLockQueue<int> c;
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
            c.send(i);
        }
    }
    int s = 0;
    for (int i = 0; i < 100 * 100; i++) {
        int tmp;
        c.receive(tmp);
        s += tmp;
    }
}

TEST(ChannelTest, LockFreeChannelSingleRW) {
    run_test_concurrent_single_rw<LockFreeChannel4096>();
}
TEST(ChannelTest, BasicChannelSingleRW) {
    run_test_concurrent_single_rw<BasicLockQueue>();
}
TEST(ChannelTest, SenderSideLockQueueSingleRW) {
    run_test_concurrent_single_rw<SenderSideLockQueue>();
}

TEST(ChannelTest, BasicChannelConcurrentWrite) {
    run_test_concurrent_multi_write<BasicLockQueue>();
}

TEST(ChannelTest, SenderSideLockQueueConcurrentWrite) {
    run_test_concurrent_multi_write<SenderSideLockQueue>();
}

template <template <class> class C>
void run_senario() {
    C<string> c2;

    auto fib = [&](int n) -> int {  // not correct just for test
        C<int> f_c1;
        C<int> f_c2;
        f_c1.send(1);
        thread f1{[&] {
            int ownstate = 0;
            int r;
            for (int i = 0; i < n / 2; i++) {
                f_c1.receive(r);
                ownstate += r;
                f_c2.send(ownstate);
            }
        }};
        thread f2{[&] {
            int ownstate = 0;
            int r;
            for (int i = 0; i < n / 2; i++) {
                f_c2.receive(r);
                ownstate += r;
                f_c1.send(ownstate);
            }
        }};
        f1.join();
        f2.join();
        int ret;
        f_c1.receive(ret);
        return ret;
    };

    ASSERT_EQ(fib(4), 3);
    ASSERT_EQ(fib(14), 377);
    ASSERT_EQ(fib(40), 102334155);
}

TEST(ChannelTest, Senario) {
    run_senario<BasicLockQueue>();
    run_senario<LockFreeChannel4096>();
    run_senario<SenderSideLockQueue>();
}
