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

template <class Model>
void logic_impl(Model* m, int conn_id, const message::ReadOnlyBuffer& buf) {
    if (m->round++ >= m->max_round) {
        m->disconnect(conn_id);
        m->stop();
    } else {
        this_thread::sleep_for(m->restime);
        message::CopyableBuffer b{buf.data(), buf.data() + buf.size()};
        m->send(0, message::ZeroCopyBuffer(move(b)));
    }
}

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

class HeartBeatServerImpl : public ServerModel{
    const chrono::milliseconds restime;
    const int max_round;
    int round;
    template <class Model>
    friend void logic_impl(Model* m, int conn_id,
                          const message::ReadOnlyBuffer& buf);

  public:
    HeartBeatServerImpl(const chrono::milliseconds resttime,
                        const int max_round)
        : restime(resttime), max_round(max_round), round(0) {}

  protected:
    void logic(int conn_id, const message::ReadOnlyBuffer& buf) override {
        logic_impl(this, conn_id, buf);
    }
};

class HeartBeatClientImpl : public ClientModel {
    const chrono::milliseconds restime;
    const int max_round;
    int round;
    template <class Model>
    friend void logic_impl(Model* m, int conn_id,
                          const message::ReadOnlyBuffer& buf);

  public:
    HeartBeatClientImpl(const chrono::milliseconds resttime,
                        const int max_round)
        : restime(resttime), max_round(max_round), round(0) {}

  protected:
    void logic(int conn_id, const message::ReadOnlyBuffer& buf) override {
        logic_impl(this, conn_id, buf);
    }
    void init_logic() override {
        string s = "hello";
        message::CopyableBuffer b{s.begin(), s.end()};
        send(0, message::ZeroCopyBuffer(move(b)));
    }
};

void setup(const chrono::milliseconds restime, const int max_round) {
    atomic_int port;
    atomic_flag ready{};
    ready.test_and_set();
    auto run_server = [&]() {
        execution_context ctx{1};
        std::error_code ec;
        ti_socket_t skt = {0, resolve("127.0.0.1", ec)};
        AsyncServer s{ctx, skt};
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
        AsyncClient c(ctx, "127.0.0.1", p);
        HeartBeatClientImpl c_impl(restime, max_round);
        RunClient(c_impl, c);
        ctx.run();
    };
    thread t1{run_server};
    thread t2{run_client};
    t1.join();
    t2.join();
}
