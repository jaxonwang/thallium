#include "function_object.hpp"

#include <boost/any.hpp>
#include <catch2/catch.hpp>

TEST_CASE("Test deSerializeArguments template function") {
    auto s = thallium::FunctionSingnature<int, int, float, double>{};
    auto bvs =
        s.deSerializeArguments(std::vector<std::string>{"2", "3.14", "5.453"});
    REQUIRE(boost::any_cast<int>(bvs[0]) == 2);
    REQUIRE(boost::any_cast<float>(bvs[1]) == (float)3.14);
    REQUIRE(boost::any_cast<double>(bvs[2]) == (double)5.453);

    auto s1 = thallium::FunctionSingnature<int, int, float, double, std::string,
                                           long double>{};
    REQUIRE_THROWS_WITH(
        s1.deSerializeArguments(std::vector<std::string>{"1", "2", "3", "4"}),
        "Function arguments arity doesnt match: given 4, need 5");
    REQUIRE_THROWS_WITH(
        s1.deSerializeArguments(std::vector<std::string>{"1", "2", "3", "4",
                                                         "5", "6"}),
        "Function arguments arity doesnt match: given 6, need 5");
}
