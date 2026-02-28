#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("public umbrella include works", "[public]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto b = Series::zeros({1,3}, ScalarType::Float64);
  auto c = a + b;
  REQUIRE(c.sizes()[0] == 2);
  REQUIRE(c.sizes()[1] == 3);
}
