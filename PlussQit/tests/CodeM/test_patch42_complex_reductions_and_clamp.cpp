#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <complex>

TEST_CASE("complex mean works", "[complex][reduce]") {
  using c64 = std::complex<float>;
  std::vector<c64> v = {c64(1,2), c64(3,4), c64(5,6), c64(7,8)};
  auto x = pluss::make_series(v, {2,2}, {{"dtype","complex64"}});
  auto m = x.mean();
  REQUIRE(m.scalar_type() == pluss::ScalarType::Complex64);
}

TEST_CASE("clamp and relu methods", "[clamp][relu]") {
  auto x = pluss::value(std::vector<float>{-1, 0.5f, 2.0f, -3.0f}, {2,2});
  auto y = x.clamp(0.0, 1.0);
  REQUIRE(y.scalar_type() == pluss::ScalarType::Float32);
  auto z = x.relu();
  REQUIRE(z.scalar_type() == pluss::ScalarType::Float32);
}
