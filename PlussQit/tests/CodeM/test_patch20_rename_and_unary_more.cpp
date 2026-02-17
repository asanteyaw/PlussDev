#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("rename cat/stack to unite/pile", "[rename]") {
  Series a = pluss::arange(0, 3, 1, ScalarType::Int64);
  Series b = pluss::arange(3, 6, 1, ScalarType::Int64);
  auto c = a.unite({b}, 0);
  REQUIRE(c.numel() == 6);
  auto p = a.pile({b}, 0);
  REQUIRE(p.dim() == 2);
}

TEST_CASE("more unary ops", "[unary_more]") {
  Series x = pluss::full({4}, 0.5, ScalarType::Float32);
  auto y = x.sigmoid();
  auto* yp = reinterpret_cast<const float*>(y.raw_data());
  REQUIRE(yp[0] > 0.6f);
  x.relu_();
  auto* xp = reinterpret_cast<const float*>(x.raw_data());
  REQUIRE(xp[0] == 0.5f);
}
