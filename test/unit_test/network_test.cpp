#include "network/network.hpp"

#include <atomic>
#include <functional>

#include "logging.hpp"
#include "test.hpp"

using namespace thallium;
using namespace std;

TEST(UtilsTest, ValidHostname) {
    ASSERT_TRUE(valid_hostname("abcdefg"));
    ASSERT_TRUE(valid_hostname("abcdefg.fdsaf"));
    ASSERT_TRUE(valid_hostname("a314bcdefg.fdsaf"));
    ASSERT_TRUE(valid_hostname("a314-bcdefg.fd-saf"));
    ASSERT_TRUE(valid_hostname("123-bcdefg.fd-saf.123"));
    ASSERT_TRUE(valid_hostname("123-bcdefg.fd-saf.123"));
    ASSERT_TRUE(valid_hostname("123.123-bcdefg.fd-saf.123"));
    ASSERT_TRUE(valid_hostname("456.123.123-bcdefg.fd-saf.123"));

    ASSERT_FALSE(valid_hostname(
        "1234123412341234123412341234123412341234123412341234123412341234"));
    char too_long[260] = {};
    for (int i = 0; i < 259; i++) {
        too_long[i] = 'a';
    }
    ASSERT_FALSE(valid_hostname(too_long));
    ASSERT_FALSE(valid_hostname(nullptr));
    ASSERT_FALSE(valid_hostname(""));
    ASSERT_FALSE(valid_hostname("."));
    ASSERT_FALSE(valid_hostname("dsaf..dsafsd"));
    ASSERT_FALSE(valid_hostname("-dsaf..dsafsd"));
    ASSERT_FALSE(valid_hostname(".dsaf.dsafsd"));
    ASSERT_FALSE(valid_hostname("dsaf.dsafsd."));
    ASSERT_FALSE(valid_hostname("dsaf.dsafsd.43*^%)"));
}

TEST(UtilsTest, ValidPort) {
    ASSERT_EQ(1235, port_to_int("1235"));
    ASSERT_EQ(-1, port_to_int(nullptr));
    ASSERT_EQ(1, port_to_int("1"));
    ASSERT_EQ(65535, port_to_int("65535"));
    ASSERT_EQ(-1, port_to_int("0"));
    ASSERT_EQ(-1, port_to_int("-123"));
    ASSERT_EQ(-1, port_to_int("a1234"));
    ASSERT_EQ(-1, port_to_int("1234k"));
    ASSERT_EQ(-1, port_to_int("65536"));
    ASSERT_EQ(-1, port_to_int("165536"));
}

class ServerImpl : public ServerModel {
    long long state;

  public:
    ServerImpl() : state(1) {}
    void logic(int conn_id, const message::ReadOnlyBuffer& buf) override {
        string s(buf.data(), buf.size());
        if (s == "stop") {
            stop();
        } else {
            state += stoll(s);
            s = to_string(state);
            message::CopyableBuffer b{s.begin(), s.end()};
            send(conn_id, message::ZeroCopyBuffer(move(b)));
        }
    }
};

class ClientImpl : public ClientModel {
    int n;
    int count;
    int state;
    function<void(long long)> f;

  public:
    ClientImpl(long long n, function<void(long long)> f)
        : n(n), count(0), state(1), f(f) {}

  protected:
    void logic(int conn_id, const message::ReadOnlyBuffer& buf) override {
        string s1(buf.data(), buf.size());
        count++;
        long long in = stoll(s1);
        if (count >= n) {
            state = in;
        stop:
            string s = "stop";
            message::CopyableBuffer b{s.begin(), s.end()};
            send(0, message::ZeroCopyBuffer(move(b)));
            disconnect(conn_id);
            stop();
            f(state);
            return;
        }
        state += in;
        count++;
        if (count >= n) goto stop;
        string s = to_string(state);
        message::CopyableBuffer b{s.begin(), s.end()};
        send(0, message::ZeroCopyBuffer(move(b)));
    }

    void init_logic() override {
        string s = "1";
        message::CopyableBuffer b{s.begin(), s.end()};
        send(0, message::ZeroCopyBuffer(move(b)));
    }
};

int distributed_fibonacci(const int n) {
    atomic_int port;
    atomic_flag ready{};
    ready.test_and_set();
    long long ret;
    auto run_server = [&]() {
        execution_context ctx{1};
        std::error_code ec;
        ti_socket_t skt = {0, resolve("127.0.0.1", ec)};
        AsyncServer s{ctx, skt};
        ServerImpl s_impl;
        RunServer(s_impl, s);
        port.store(s.server_socket().port);
        ready.clear();
        ctx.run();
    };
    auto run_client = [&]() {
        execution_context ctx{1};
        while (ready.test_and_set()) {
        }  // spin
        int p = port.load();
        AsyncClient c(ctx, "127.0.0.1", p);
        ClientImpl c_impl(
            n, function<void(long long)>{[&](long long r) { ret = r; }});
        RunClient(c_impl, c);
        ctx.run();
    };
    thread t1{run_server};
    thread t2{run_client};
    t1.join();
    t2.join();
    return ret;
}

TEST(Integrated, Fibonacci) {
    ti_test::LoggingTracer _t{1, true};
    ASSERT_EQ(distributed_fibonacci(40), 267914296);
}

template <class Model>
void logic_impl(Model* m, int conn_id, const message::ReadOnlyBuffer& buf,
                bool need_disconnect) {
    if (m->round++ >= m->max_round) {
        if (need_disconnect)
            m->disconnect(conn_id);
        else {
            std::this_thread::sleep_for(chrono::milliseconds{5});
        }
        m->stop();
        cout << "done " << endl;
    } else {
        this_thread::sleep_for(m->restime);
        message::CopyableBuffer b{buf.data(), buf.data() + buf.size()};
        m->send(0, message::ZeroCopyBuffer(move(b)));
    }
}

class HeartBeatServerImpl : public ServerModel {
    const chrono::milliseconds restime;
    const int max_round;
    int round;
    template <class Model>
    friend void logic_impl(Model*, int, const message::ReadOnlyBuffer&, bool);

  public:
    HeartBeatServerImpl(const chrono::milliseconds resttime,
                        const int max_round)
        : restime(resttime), max_round(max_round + 1), round(0) {}

  protected:
    void event(int, const message::ConnectionEvent& e) override {
        switch (e) {
            case message::ConnectionEvent::timeout:
                stop();
                break;
            case message::ConnectionEvent::close:
                break;
            case message::ConnectionEvent::pipe:
                break;
            default:
                throw std::logic_error("Should't be here!");
        }
    }

    void logic(int, const message::ReadOnlyBuffer& buf) override {
        // cout << "server" << round << endl;
        if (round++ >= max_round) {
            stop();
            // cout << "done " << endl;
        } else {
            this_thread::sleep_for(restime);
            message::CopyableBuffer b{buf.data(), buf.data() + buf.size()};
            send(0, message::ZeroCopyBuffer(move(b)));
        }
    }
};

class HeartBeatClientImpl : public ClientModel {
    const chrono::milliseconds restime;
    const int max_round;
    int round;
    template <class Model>
    friend void logic_impl(Model*, int, const message::ReadOnlyBuffer&, bool);

  public:
    HeartBeatClientImpl(const chrono::milliseconds resttime,
                        const int max_round)
        : restime(resttime), max_round(max_round), round(0) {}

  protected:
    void logic(int conn_id, const message::ReadOnlyBuffer& buf) override {
        this_thread::sleep_for(restime);
        message::CopyableBuffer b{buf.data(), buf.data() + buf.size()};
        send(0, message::ZeroCopyBuffer(move(b)));
        // cout << "client" << round << endl;
        if (round++ >= max_round) {
            disconnect(conn_id);
            stop();
            // cout << "done " << endl;
        }
    }
    void init_logic() override {
        string s = "hello";
        message::CopyableBuffer b{s.begin(), s.end()};
        send(0, message::ZeroCopyBuffer(move(b)));
    }
};

void heartbeatsetup(const chrono::milliseconds restime, const int max_round) {
    atomic_int port;
    atomic_flag ready{};
    ready.test_and_set();
    auto run_server = [&]() {
        execution_context ctx{1};
        std::error_code ec;
        ti_socket_t skt = {0, resolve("127.0.0.1", ec)};
        AsyncServer s{ctx, skt, true};
        HeartBeatServerImpl s_impl(restime, max_round);
        RunServer(s_impl, s);
        port.store(s.server_socket().port);
        ready.clear();
        ctx.run();
    };
    auto run_client = [&]() {
        execution_context ctx{1};
        while (ready.test_and_set()) {
        }  // spin
        int p = port.load();
        AsyncClient c(ctx, "127.0.0.1", p, true);
        HeartBeatClientImpl c_impl(restime, max_round);
        RunClient(c_impl, c);
        ctx.run();
    };
    thread t1{run_server};
    thread t2{run_client};
    t1.join();
    t2.join();
}

void setHeartBeatPolicy(const int interval, const int timeout,
                        const int init_timeout, bool revert = false) {
    static chrono::milliseconds saved_interval;
    static chrono::milliseconds saved_timeout;
    static chrono::milliseconds saved_init;

    const chrono::milliseconds _interval{interval};
    const chrono::milliseconds _timeout{timeout};
    const chrono::milliseconds _init_timeout{init_timeout};
    if (!revert) {
        saved_interval = HeartBeatPolicy::interval;
        saved_timeout = HeartBeatPolicy::timeout;
        saved_init = HeartBeatPolicy::server_initial_timeout;
        HeartBeatPolicy::interval = _interval;
        HeartBeatPolicy::timeout = _timeout;
        HeartBeatPolicy::server_initial_timeout = _init_timeout;
    } else {
        HeartBeatPolicy::interval = saved_interval;
        HeartBeatPolicy::timeout = saved_timeout;
        HeartBeatPolicy::server_initial_timeout = saved_init;
    }
}

void revertHeartBeatPolicy() { setHeartBeatPolicy(0, 0, 0, true); }

bool contain(const string& s, const string& p) {
    return s.find(p) != string::npos;
}

TEST(HeartBeat, Basic) {
    ti_test::LoggingTracer t{0, false};
    // need to make sure client will not time out
    // but very time client sending the heartbeat will write logs
    // io may cause time out.
    // Here I use /dev/shm, otherwise I need to set a huge interval
    setHeartBeatPolicy(50, 100, 150);
    heartbeatsetup(chrono::milliseconds{1}, 50);
    revertHeartBeatPolicy();

    auto logs = t.stop_and_collect();
    auto full_log = string_join(logs, "\n");

    ASSERT_TRUE(logs.size() > 7) << full_log;

    const string s1{"Accepting connection from"};
    const string s2{"Successfully connect to"};
    // order might differ
    ASSERT_TRUE((contain(logs[1], s1) && contain(logs[0], s2)) ||
                (contain(logs[0], s1) && contain(logs[1], s2))) << full_log;

    for (size_t i = 2; i < logs.size() - 3; i++) {
        ASSERT_TRUE(contain(logs[i], "Send heartbeat")) << full_log;
        ASSERT_TRUE((logs[++i], "Receive heartbeat")) << full_log;
    }

    const string s3{"Acceptor stopped"};
    const string s4{"Operation canceled"};
    const string s5{"closes connection"};
    int matched = 0;
    for (int i = 1; i <= 3; i++) {
        const string& _s = logs[logs.size() - i];
        if (contain(_s, s3) || contain(_s, s4) || contain(_s, s5)) matched++;
    }
    ASSERT_EQ(matched, 3) << full_log;

    for (auto& i : logs) {
        ASSERT_FALSE(contain(i, "ERROR")) << full_log;
    }
}

TEST(HeartBeat, ClientTimeout) {
    ti_test::LoggingTracer t{0, false};
    setHeartBeatPolicy(5, 5, 20);
    heartbeatsetup(chrono::milliseconds{1}, 10);
    revertHeartBeatPolicy();

    auto logs = t.stop_and_collect();
    auto full_log = string_join(logs, "\n");

    const string s1{"Accepting connection from"};
    const string s2{"Successfully connect to"};
    // order might differ
    ASSERT_TRUE((contain(logs[1], s1) && contain(logs[0], s2)) ||
                (contain(logs[0], s1) && contain(logs[1], s2))) << full_log;
    int error_log_num = 0;

    for (auto& i : logs) {
        if (contain(i, "ERROR")) error_log_num++;
    }
    ASSERT_TRUE(error_log_num <= 2) << full_log;
}
