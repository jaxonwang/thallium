#include "function_object.hpp"

#include <catch2/catch.hpp>

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
    auto f = [](int a, int b, int d, std::string c, int e) {
        return a + b + d + e + c.size();
    };
    std::function<int(int, int, int, std::string, int)> f1 = f;

    auto fobj = make_function_object(f1);
    vector<BoxedValue> bvs;
    int a = 1, b = 1, d = 3, e=4;
    string c = "fdas";
    bvs.push_back(move(a));
    bvs.push_back(move(b));
    bvs.push_back(move(d));
    bvs.push_back(move(c));
    bvs.push_back(move(e));
    // std::cout << BoxedValue::boxCast<int>(bvs[0]) << std::endl;
    // std::cout << BoxedValue::boxCast<int>(bvs[1]) << std::endl;
    // std::cout << BoxedValue::boxCast<int>(bvs[2]) << std::endl;
    // std::cout << BoxedValue::boxCast<std::string>(bvs[3]) << std::endl;
    // std::cout << BoxedValue::boxCast<int>(bvs[4]) << std::endl;
    // std::cout << BoxedValue::boxCast<int>(bvs[0]) << std::endl;
    // std::cout << "sdfasdfdasffffffff"<<endl;
    //
    BoxedValue bv = fobj(bvs);
    REQUIRE( *BoxedValue::boxCast<int>(bv) == 13);
}
