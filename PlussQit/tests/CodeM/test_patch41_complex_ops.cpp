#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <complex>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("complex arithmetic and unary math", "[complex]") {
  using c64 = std::complex<float>;
  std::vector<c64> v = {c64(1,2), c64(3,4), c64(-1,0), c64(0.5f,-0.5f)};
  auto x = pluss::make_series(v, {2,2}, {{"dtype","complex64"}});
  auto y = x.add(x).sub(x).mul(x).div(x); // should be x
  REQUIRE(y.scalar_type() == pluss::ScalarType::Complex64);

  auto e = x.exp();
  REQUIRE(e.scalar_type() == pluss::ScalarType::Complex64);

  auto r = x.real();
  auto im = x.imag();
  REQUIRE(r.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(im.scalar_type() == pluss::ScalarType::Float32);

  auto cj = x.conj();
  REQUIRE(cj.scalar_type() == pluss::ScalarType::Complex64);

  auto ang = x.angle();
  REQUIRE(ang.scalar_type() == pluss::ScalarType::Float32);
}

TEST_CASE("complex comparisons eq/ne only", "[complex]") {
  using c128 = std::complex<double>;
  std::vector<c128> v = {c128(1,2), c128(1,2), c128(0,0), c128(0,1)};
  auto x = pluss::make_series(v, {2,2}, {{"dtype","complex128"}});
  auto y = x.clone();
  auto m = x.eq(y);
  REQUIRE(m.scalar_type() == pluss::ScalarType::Bool);
  auto n = x.ne(y);
  REQUIRE(n.scalar_type() == pluss::ScalarType::Bool);
}
