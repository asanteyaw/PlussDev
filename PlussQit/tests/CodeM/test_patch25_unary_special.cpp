#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("special unary ops implemented", "[unary_special]") {
  Series x = pluss::full({4}, 1.0, ScalarType::Float32);
  auto y = x.log1p();
  auto* yp = reinterpret_cast<const float*>(y.raw_data());
  REQUIRE(yp[0] > 0.6f);

  auto z = x.expm1();
  auto* zp = reinterpret_cast<const float*>(z.raw_data());
  REQUIRE(zp[0] > 1.6f);

  auto f = x.erf();
  REQUIRE(f.scalar_type() == ScalarType::Float32);

  auto m = x.isnan();
  REQUIRE(m.scalar_type() == ScalarType::Bool);
}
