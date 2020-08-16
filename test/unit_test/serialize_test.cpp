#include "serialize.hpp"

#include "test.hpp"

using namespace std;
using namespace thallium;
using namespace Serializer;

TEST(Serialize, Base) {
    StringSaveArchive ar;

    int a0 = 321, a1;
    float b0 = 123.4565, b1;
    char c0 = 'K', c1;
    double d0 = -123.123 * 10e-19, d1;
    long e0 = -1, e1;

    ar << a0 << b0 << c0 << d0 << e0;
    string s = ar.build();
    StringLoadArchive la(s);
    la >> a1 >> b1 >> c1 >> d1 >> e1;
    ASSERT_EQ(a0, a1);
    ASSERT_EQ(b0, b1);
    ASSERT_EQ(c0, c1);
    ASSERT_EQ(d0, d1);
    ASSERT_EQ(e0, e1);
}

TEST(Serialize, Array) {
    StringSaveArchive ar;

    constexpr int arr_len = 128;
    int arr0[arr_len];
    double arr1[arr_len];

    for (int i = 0; i < arr_len; i++) {
        arr0[i] = i;
        arr1[i] = i * 1000 / 7.0;
    }

    char brk = ' ';
    ar << arr0 << brk << arr1;
    string s = ar.build();

    StringLoadArchive la(s);
    int arr0_cpy[arr_len];
    double arr1_cpy[arr_len];
    la >> arr0_cpy >> brk >> arr1_cpy;

    for (int i = 0; i < arr_len; i++) {
        ASSERT_EQ(arr0[i], arr0_cpy[i]);
        ASSERT_EQ(arr1[i], arr1_cpy[i]);
    }
}

class Inner {
  private:
    static constexpr int len = 123;
    int a[len];

  public:
    Inner() = default;
    Inner(int init_value) {
        for (int i = 0; i < len; i++) {
            a[i] = init_value * i;
        }
    }
    bool operator==(const Inner& other) const {
        for (int i = 0; i < len; i++) {
            if (this->a[i] != other.a[i]) return false;
        }
        return true;
    }
    template <class Archive>
    void serializable(Archive& ar) {
        ar& a;
    }
};

class ToBeTest {
  private:
    bool a;
    short b;
    char c;
    int d;
    long e;
    float f;
    double g;
    long double h;
    long long i;
    Inner j;

  public:
    ToBeTest() = default;
    ToBeTest(bool a, short b, char c, int d, long e, float f, double g,
             long double h, long long i)
        : a(a), b(b), c(c), d(d), e(e), f(f), g(g), h(h), i(i), j(d) {}
    template <class Archive>
    void serializable(Archive& ar) {
        ar& a;
        ar& b;
        ar& c;
        ar& d;
        ar& e;
        ar& f;
        ar& j;
        ar& g;
        ar& h;
        ar& i;
    }
    bool operator==(const ToBeTest& other) const {
        bool ret = this->a == other.a && this->b == other.b &&
                   this->c == other.c && this->d == other.d &&
                   this->e == other.e && this->f == other.f &&
                   this->g == other.g && this->h == other.h &&
                   this->i == other.i && this->j == other.j;
        return ret;
    }
};

TEST(Serialize, ClassAndSturct) {
    ToBeTest t1(true, -213, 'k', 4325436, 80321475, 4231.0 / 7.0,
                4324125.0 / 13.0, 10e15 / 17.0, 77676726347);
    ToBeTest t2;

    StringSaveArchive sa;
    sa << t1;
    string s = sa.build();
    StringLoadArchive la{s};
    la >> t2;
    ASSERT_EQ(t1, t2);
}

TEST(Serialize, String) {
    int a0 = 13214, b0 = 32542;
    int a1, b1;
    string s1{"1242305843190680134"};
    string s2;
    StringSaveArchive sa;
    sa << a0;
    sa << s1;
    sa << b0;
    string s = sa.build();
    StringLoadArchive la{s};
    la >> a1;
    la >> s2;
    la >> b1;
    ASSERT_EQ(s1, s2);
    ASSERT_EQ(a0, a1);
    ASSERT_EQ(b0, b1);
}

TEST(Serialize, Vector) {
    vector<int> v_i1, v_i2;
    vector<string> v1, v2;
    vector<ToBeTest> v_c1, v_c2;
    for (int i = 0; i < 20; i++) {
        v_i1.push_back(i);
        v1.push_back(to_string(i * i * 10000));
        v_c1.push_back({true, static_cast<short>(2 * i), static_cast<char>(i),
                        i << 4, i << 5, 7.1235, i * i * i / 13.0,
                        10e15 * i / 17.0, i * i << 4});
    }
    StringSaveArchive sa;
    sa << v_i1;
    sa << v1;
    sa << v_c1;
    string s = sa.build();
    StringLoadArchive la{s};
    la >> v_i2;
    la >> v2;
    la >> v_c2;
    ASSERT_EQ(v1, v2);
    ASSERT_EQ(v_i1, v_i2);
    ASSERT_EQ(v_c1, v_c2);
}

TEST(Serializer, RealSize) {
    long long a = 1;
    ASSERT_EQ(sizeof(a), real_size(a));

    double b[28];
    ASSERT_EQ(sizeof(b), real_size(b));

    class ToBeTest t;
    size_t class_size = real_size(t);
    size_t expected = sizeof(bool) + sizeof(short) + sizeof(char) +
                      sizeof(int) + sizeof(long) + sizeof(float) +
                      sizeof(double) + sizeof(long double) + sizeof(long long);
    expected += real_size(Inner());
    ASSERT_EQ(class_size, expected);

    const size_t size_t_size = sizeof(size_t);

    const ToBeTest ts[13]{};
    ASSERT_EQ(real_size(ts), 13 * class_size);

    const string s{"1234567"};
    ASSERT_EQ(real_size(s), 7 + size_t_size);

    auto sp1 = gsl::make_span(b);
    ASSERT_EQ(real_size(sp1), sizeof(b) + size_t_size);

    auto sp2 = gsl::make_span(ts);
    ASSERT_EQ(real_size(sp2), expected * 13 + size_t_size);

    const vector<int> v1 = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ASSERT_EQ(real_size(v1), sizeof(int) * 9 + size_t_size);

    vector<string> v2;
    size_t v2_len = 12;
    for (long long i = 1, m = 1; i <= static_cast<long long>(v2_len); i++) {
        v2.push_back(to_string(m));
        m *= 10;
    }
    ASSERT_EQ(real_size(v2), (1 + v2_len) * v2_len / 2 * sizeof(char) +
                                 v2_len * size_t_size + size_t_size);
}
