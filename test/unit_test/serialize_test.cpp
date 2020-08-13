#include "serialize.hpp"
#include "test.hpp"

using namespace std;
using namespace thallium;
using namespace Serializer;


TEST(Serialize, Base){

    StringSaveArchive ar;

    int a0 = 321, a1;
    float b0 = 123.4565, b1;
    char c0 = 'K', c1;
    double d0 = -123.123 * 10e-19 ,d1 ;
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

TEST(Serialize, Compund){

    StringSaveArchive ar;

    constexpr int arr_len = 128;
    int arr0[arr_len];
    double arr1[arr_len];

    for (int i = 0; i < arr_len; i++) {
        arr0[i] = i ;
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
