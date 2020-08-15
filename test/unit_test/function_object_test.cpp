
#include "function_object.hpp"

#include <memory>
#include <string>

#include "test.hpp"
#include "thallium.hpp"

using namespace thallium;
using namespace thallium::Serializer;

TEST(FunctionObjectTest, DeSerializeArgumentsTemplateFunction) {
    auto s = thallium::FunctionSingnature<int(int, float, double)>{};
    auto bvs = s.deSerializeArguments(
        std::vector<std::string>{create_string(2), create_string<float>(3.14),
                                 create_string<double>(5.453)});
    ASSERT_TRUE(*BoxedValue::boxCast<int>(bvs[0]) == 2);
    ASSERT_TRUE(*BoxedValue::boxCast<float>(bvs[1]) == (float)3.14);
    ASSERT_TRUE(*BoxedValue::boxCast<double>(bvs[2]) == (double)5.453);

    auto s1 = thallium::FunctionSingnature<int(int, float, double, std::string,
                                               long double)>{};
    auto str0 = create_string(1);
    auto str1 = create_string<float>(1.0);
    auto str2 = create_string<double>(1.0);
    auto str3 = create_string(string{"1234"});
    auto str4 = create_string<long double>(1.0);
    ASSERT_THROW_WITH(s1.deSerializeArguments(
                          std::vector<std::string>{str0, str1, str2, str3}),
                      "Function arguments arity doesnt match: given 4, need 5");
    ASSERT_THROW_WITH(s1.deSerializeArguments(std::vector<std::string>{
                          str0, str1, str2, str3, str4, str4}),
                      "Function arguments arity doesnt match: given 6, need 5");
}

TEST(FunctionObjectTest, FunctionObjectTestBindCVREF) {
    auto f1 = [](const int a, const int &b, int d, std::string &c, int e,
                 std::string &&f) {
        return a + b + d + e + c.size() + f.size();
    };
    std::function<int(const int, const int &, int, std::string &, int,
                      std::string &&)>
        f1_ = f1;
    auto fobj = make_function_object(f1_);

    vector<BoxedValue> bvs;
    int a = 1, b = 1, d = 3, e = 4;
    string c = "fdas";
    string f = "hahah";
    bvs.push_back(BoxedValue(a));
    bvs.push_back(BoxedValue(move(b)));
    bvs.push_back(BoxedValue(d));
    bvs.push_back(BoxedValue(c));
    bvs.push_back(BoxedValue(e));
    bvs.push_back(BoxedValue(f));
    BoxedValue bv = fobj(bvs);
    ASSERT_TRUE(*BoxedValue::boxCast<int>(bv) == 18);
}

TEST(FunctionObjectTest, FunctionCall) {
    auto f1 = [](int a, int b, int d, std::string c, int e) {
        return a + b + d + e + c.size();
    };
    std::function<int(int, int, int, std::string, int)> f1_ = f1;
    auto fobj = make_function_object(f1_);

    vector<BoxedValue> bvs;
    int a = 1, b = 1, d = 3, e = 4;
    string c = "fdas";
    bvs.push_back(BoxedValue(a));
    bvs.push_back(BoxedValue(move(b)));
    bvs.push_back(BoxedValue(d));
    bvs.push_back(BoxedValue(c));
    bvs.push_back(BoxedValue(e));
    BoxedValue bv = fobj(bvs);
    ASSERT_TRUE(*BoxedValue::boxCast<int>(bv) == 13);

    auto f2 = [](int b, int d, std::string c, int e) {
        return b + d + e + c.size();
    };
    std::function<int(int, int, std::string, int)> f2_ = f2;
    ASSERT_THROW_WITH(make_function_object(f2_)(bvs),
                      "Function arguments arity doesnt match: given 5, need 4");

    auto f3 = [](int a, int b, int d, std::string c, int e, int f) {
        return a + b + d + e + f + c.size();
    };
    auto bvs3 = bvs;
    std::function<int(int, int, int, std::string, int, int)> f3_ = f3;
    ASSERT_THROW_WITH(make_function_object(f3_)(bvs3),
                      "Function arguments arity doesnt match: given 5, need 6");

    unique_ptr<int> p{new int(2)};
    BoxedValue ptr{move(p)};

    struct testsp {
        char load[100];
    };
    auto sp = make_shared<testsp>();
    BoxedValue tmp{sp};
}

int func_test(int a, int b, int d, std::string c, int e) {
    return a * b * d * c.size() * e;
}

TEST(FunctionObjectTest, FunctionManager) {
    auto f1 = [](int a, int b, int d, std::string c, int e) {
        return a + b + d + e + c.size();
    };
    std::function<int(int, int, int, std::string, int)> f1_ = f1;
    register_func(f1_);
    auto fobj = thallium::get_function_object(function_id(f1_));

    vector<BoxedValue> bvs;
    int a = 1, b = 1, d = 3, e = 4;
    string c = "fdas";
    bvs.push_back(BoxedValue(a));
    bvs.push_back(BoxedValue(move(b)));
    bvs.push_back(BoxedValue(d));
    bvs.push_back(BoxedValue(c));
    bvs.push_back(BoxedValue(e));
    BoxedValue bv = (*fobj)(bvs);
    ASSERT_TRUE(*BoxedValue::boxCast<int>(bv) == 13);

    register_func(func_test);
    auto fobj2 = thallium::get_function_object(function_id(func_test));
    BoxedValue bv2 = (*fobj2)(bvs);
    ASSERT_TRUE(*BoxedValue::boxCast<int>(bv2) == 48);
    ASSERT_THROW(register_func(func_test), std::logic_error);
}

int funtest(int a) { return a + 1; }

TEST(FinishTest, FinishSuger) {
    auto bs_ptr = BuffersPtr{new Buffers{"123", "321", "1234567"}};
    for (FinishSugar i{}; i.once; i.once = false) {
        AsyncExecManager::get().submitExecution(this_host, function_id(funtest),
                                                move(bs_ptr));
        AsyncExecManager::get().submitExecution(this_host, function_id(funtest),
                                                move(bs_ptr));
        AsyncExecManager::get().submitExecution(this_host, function_id(funtest),
                                                move(bs_ptr));
    }
    ASSERT_THROW(AsyncExecManager::get().submitExecution(
                     this_host, function_id(funtest), move(bs_ptr)),
                 std::logic_error);
}
