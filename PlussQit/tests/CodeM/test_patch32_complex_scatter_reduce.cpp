#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <complex>

using pluss::codem::Series;
using pluss::codem::ScalarType;

TEST_CASE("complex add/mul + exp/log", "[complex]") {
  pluss::Series a = pluss::full({2}, 0.0, pluss::ScalarType::Complex64);
  auto* ap = reinterpret_cast<std::complex<float>*>(a.raw_data());
  ap[0] = {1.0f, 2.0f};
  ap[1] = {0.0f, -1.0f};
  pluss::Series b = a.mul(a).add(a);
  REQUIRE(b.scalar_type() == pluss::ScalarType::Complex64);
  pluss::Series e = a.exp();
  REQUIRE(e.scalar_type() == pluss::ScalarType::Complex64);
  pluss::Series l = e.log();
  REQUIRE(l.scalar_type() == pluss::ScalarType::Complex64);
}
