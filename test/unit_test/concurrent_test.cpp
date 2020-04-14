#include "concurrent.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "test.hpp"

TEST(ChannelTest, Basic) {
    using namespace thallium;
    using namespace std;
    const int arr_size = 7;
    int a[arr_size] = {7, 6, 5, 4, 3, 2, 1};

    Channel<int> c;
    for (auto &e : a) {
        c.send(e);
    }
    for (int i = 0; i < arr_size; i++) {
        int r;
        c.receive(r);
        ASSERT_EQ(a[i], r);
    }

    Channel<string> c1;
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
}

TEST(ChannelTest, concurrent) {
    using namespace thallium;
    using namespace std;
    Channel<string> c;
    Channel<string> c1;
    Channel<string> c2;

    chrono::milliseconds wait_time{100};
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
    thread t3{[&]() { c1.send(string{"just test"}); }};

    auto fib = [&](int n)->int{ //not correct just for test
        Channel<int> f_c1;
        Channel<int> f_c2;
        f_c1.send(1);
        thread f1{[&]{
            int ownstate = 0;
            int r;
            for (int i = 0; i < n/2; i++) {
                f_c1.receive(r);
                ownstate += r;
                f_c2.send(ownstate);
            }
        }};
        thread f2{[&]{
            int ownstate = 0;
            int r;
            for (int i = 0; i < n/2; i++) {
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

    auto sum= [&](int n)->int{ //not correct just for test
        n++;
        Channel<int> c;
        int ret = 0;
        vector<thread> ts;
        for (int i = 0; i < n; i++) {
           ts.push_back(thread{[=, &c]{
                   this_thread::sleep_for(chrono::milliseconds(10));
                   c.send(i);}}
                   );
        }
        thread t_sum{[&]{
            int s= 0;
            for (int i = 0; i < n; i++) {
               int tmp; 
               c.receive(tmp);
               s+=tmp;
            }
            ret = s;
        }};
        t_sum.join();
        for (auto &t: ts) {
           t.join(); 
        }
        return ret;
    };

    t1.join();
    t2.join();
    t3.join();
    ASSERT_EQ(t1_ret, 4321);
    ASSERT_EQ(t2_s, "just test");
    ASSERT_EQ(fib(4), 3);
    ASSERT_EQ(fib(14), 377);
    ASSERT_EQ(fib(40), 102334155);
    ASSERT_EQ(sum(100), 5050);
}
