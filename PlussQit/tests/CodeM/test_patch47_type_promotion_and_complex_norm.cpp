#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <complex>

TEST_CASE("type promotion: int + float -> float", "[promotion]") {
  auto a = pluss::value(std::vector<std::int64_t>{1,2,3}, {3});
  auto b = pluss::value(std::vector<float>{1.0f,2.0f,3.0f}, {3});
  auto c = a.add(b);
  REQUIRE(c.scalar_type() == pluss::ScalarType::Float32);
}

TEST_CASE("type promotion: float64 + float32 -> float64", "[promotion]") {
  auto a = pluss::value(std::vector<double>{1,2,3}, {3});
  auto b = pluss::value(std::vector<float>{1,2,3}, {3});
  auto c = a.mul(b);
  REQUIRE(c.scalar_type() == pluss::ScalarType::Float64);
}

TEST_CASE("complex norm works", "[complex][norm]") {
  using c64 = std::complex<float>;
  std::vector<c64> v = {c64(3,4), c64(0,0)};
  auto x = pluss::make_series(v, {2}, {{"dtype","complex64"}});
  auto n = x.norm(2.0);
  REQUIRE(n.scalar_type() == pluss::ScalarType::Float32);
}
