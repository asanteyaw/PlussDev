#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <cmath>


TEST_CASE("type promotion + comparisons + NaN", "[promote][cmp][nan]") {
  Series a = pluss::arange(0, 4, 1, ScalarType::Int32);
  Series b = pluss::full({4}, 1.5, ScalarType::Float32);
  auto c = a.add(b); // promote -> Float32
  REQUIRE(c.scalar_type() == ScalarType::Float32);

  auto m = c.gt(pluss::full({1}, 2.0, ScalarType::Float32)); // broadcast scalar
  REQUIRE(m.scalar_type() == ScalarType::Bool);

  Series x = pluss::full({2}, 0.0, ScalarType::Float32);
  // make NaN
  x = x.div(pluss::full({2}, 0.0, ScalarType::Float32));
  auto isn = x.isnan();
  auto* ip = reinterpret_cast<const std::uint8_t*>(isn.raw_data());
  REQUIRE(ip[0] == 1);
}
