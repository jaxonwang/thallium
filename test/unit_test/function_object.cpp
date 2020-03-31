#include "function_object.hpp"
#include "utils.hpp"

#include <catch2/catch.hpp>
#include <memory>

using namespace thallium;

TEST_CASE("Test deSerializeArguments template function") {
    auto s = thallium::FunctionSingnature<int, int, float, double>{};
    auto bvs =
        s.deSerializeArguments(std::vector<std::string>{"2", "3.14", "5.453"});
    REQUIRE(*BoxedValue::boxCast<int>(bvs[0]) == 2);
    REQUIRE(*BoxedValue::boxCast<float>(bvs[1]) == (float)3.14);
    REQUIRE(*BoxedValue::boxCast<double>(bvs[2]) == (double)5.453);

    auto s1 = thallium::FunctionSingnature<int, int, float, double, std::string,
                                           long double>{};
    REQUIRE_THROWS_WITH(
        s1.deSerializeArguments(std::vector<std::string>{"1", "2", "3", "4"}),
        "Function arguments arity doesnt match: given 4, need 5");
    REQUIRE_THROWS_WITH(
        s1.deSerializeArguments(
            std::vector<std::string>{"1", "2", "3", "4", "5", "6"}),
        "Function arguments arity doesnt match: given 6, need 5");
}

TEST_CASE("Test Function Object") {
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
    REQUIRE(*BoxedValue::boxCast<int>(bv) == 13);

    auto f2 = [](int b, int d, std::string c, int e) {
        return b + d + e + c.size();
    };
    std::function<int(int, int, std::string, int)> f2_ = f2;
    REQUIRE_THROWS_WITH(
        make_function_object(f2_)(bvs),
        "Function arguments arity doesnt match: given 5, need 4");

    auto f3 = [](int a, int b, int d, std::string c, int e, int f) {
        return a + b + d + e + f + c.size();
    };
    auto bvs3 = bvs;
    std::function<int(int, int, int, std::string, int, int)> f3_ = f3;
    REQUIRE_THROWS_WITH(
        make_function_object(f3_)(bvs3),
        "Function arguments arity doesnt match: given 5, need 6");

    unique_ptr<int> p{new int[10]{}};
    BoxedValue ptr{move(p)};

    struct testsp{
        char load[100];
    };
    auto sp = make_shared<testsp>();
    BoxedValue tmp{sp};
}

TEST_CASE("Test Function Manager") {
    auto f1 = [](int a, int b, int d, std::string c, int e) {
        return a + b + d + e + c.size();
    };
    std::function<int(int, int, int, std::string, int)> f1_ = f1;
    register_func(f1_);
    auto fobj = thallium::getFunctionObject(function_id(f1_));

    vector<BoxedValue> bvs;
    int a = 1, b = 1, d = 3, e = 4;
    string c = "fdas";
    bvs.push_back(BoxedValue(a));
    bvs.push_back(BoxedValue(move(b)));
    bvs.push_back(BoxedValue(d));
    bvs.push_back(BoxedValue(c));
    bvs.push_back(BoxedValue(e));
    BoxedValue bv = (*fobj)(bvs);
    REQUIRE(*BoxedValue::boxCast<int>(bv) == 13);
}
