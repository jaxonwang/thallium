#include "gsl/span.hpp"

#include <array>
#include <vector>

#include "test.hpp"

using namespace std;
using namespace thallium;

template <class T, class Iterable>
void fun_using_span(const gsl::span<T> &t, Iterable &list_like) {
    vector<T> a, b;
    for (auto &i : t) {
        a.push_back(i);
    }
    for (auto &i : list_like) {
        b.push_back(i);
    }
    ASSERT_EQ(a, b);
}

TEST(Span, Constructor) {
    vector<int> v1{1, 2, 3, 4, 5};
    auto s_v1 = gsl::make_span(v1.data(), v1.size());
    for (size_t i = 0; i < v1.size(); i++) {
        ASSERT_EQ(v1[i], s_v1[i]);
    }

    const vector<int> v2{2, 4, 6, 8, 10};
    auto s_v2 = gsl::make_span(v2.begin(), v2.end());
    for (size_t i = 0; i < v2.size(); i++) {
        ASSERT_EQ(v2[i], s_v2[i]);
    }

    int v3[]{5, 6, 7, 8, 9};
    const size_t v3_len = sizeof(v3) / sizeof(int);
    auto s_v3 = gsl::make_span(v3);
    for (size_t i = 0; i < v3_len; i++) {
        ASSERT_EQ(v3[i], s_v3[i]);
    }

    string s{"abcdefghijk"};
    auto s_s = gsl::make_span(s);
    for (size_t i = 0; i < s_s.size(); i++) {
        ASSERT_EQ(s_s[i], s[i]);
    }

    const size_t len = 20;
    int v4[len];
    std::array<int, len> v4_arr;
    for (size_t i = 0; i < len; i++) {
        v4[i] = i * i;
        v4_arr[i] = i * i;
    }
    fun_using_span<int>(v4, v4_arr);
}

TEST(Span, Operation){
    vector<int> v1{1, 2, 3, 4, 5};
    auto s_v1 = gsl::make_span(v1.data(), v1.size());
    ASSERT_EQ(s_v1.size_bytes(), 5 * sizeof(int));
    auto sub_s = s_v1.first(3);
    ASSERT_EQ(sub_s.size(), 3);
    for (size_t i = 0; i < sub_s.size(); i++) {
       ASSERT_EQ(sub_s[i],i+1);
    }
    sub_s = s_v1.last(2);
    ASSERT_EQ(sub_s.size(), 2);
    for (size_t i = 0; i < sub_s.size(); i++) {
       ASSERT_EQ(sub_s[i],i+4);
    }

    sub_s = s_v1.subspan(1, 3);
    ASSERT_EQ(sub_s.empty(), false);
    ASSERT_EQ(sub_s.size(), 3);
    for (size_t i = 0; i < sub_s.size(); i++) {
       ASSERT_EQ(sub_s[i],i+2);
    }
    
    sub_s = s_v1.subspan(1, 0);
    ASSERT_EQ(sub_s.empty(), true);
    
    ASSERT_EQ(s_v1.front(), 1);
    ASSERT_EQ(s_v1.back(), 5);
    ASSERT_EQ(&s_v1.front(), s_v1.data());
    
}
