#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <cmath>


TEST_CASE("float8 storage policy + fast ops", "[float8][fast]") {
  Series a = pluss::full({4}, 1.0, ScalarType::Float8E5M2);
  auto e = a.exp();
  REQUIRE(e.scalar_type() == ScalarType::Float8E5M2);
  auto l = e.log();
  REQUIRE(l.scalar_type() == ScalarType::Float8E5M2);

  // NaN constant
  auto n = pluss::nan();
  REQUIRE(n.scalar_type() == ScalarType::Float32);
}

TEST_CASE("scatter_add basic", "[scatter]") {
  Series self = pluss::zeros({2,3}, ScalarType::Int64);
  Series src  = pluss::arange(0, 6, 1, ScalarType::Int64).view({2,3});
  Series idx  = pluss::full({2,3}, 0, ScalarType::Int64);
  // scatter along dim1: put each src element into column 0
  auto out = self.scatter_add(1, idx, src);
  auto* p = reinterpret_cast<const std::int64_t*>(out.raw_data());
  REQUIRE(p[0] == 0+1+2); // first row sum into col0
  REQUIRE(p[3] == 3+4+5); // second row sum into col0
}
