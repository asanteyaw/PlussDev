#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("linspace values correct", "[factories]") {
  auto x = pluss::linspace(0.0, 1.0, 5, {{"dtype","float64"}});
  auto xc = x.contiguous();
  const auto* p = reinterpret_cast<const double*>(xc.raw_data());
  REQUIRE(p[0] == 0.0);
  REQUIRE(p[4] == 1.0);
}

TEST_CASE("eye diagonal correct", "[factories]") {
  auto I = pluss::eye(4, 4, {{"dtype","int32"}});
  auto Ic = I.contiguous();
  const auto* p = reinterpret_cast<const std::int32_t*>(Ic.raw_data());
  for (int r=0;r<4;++r) {
    for (int c=0;c<4;++c) {
      const auto v = p[r*4 + c];
      if (r==c) REQUIRE(v == 1);
      else REQUIRE(v == 0);
    }
  }
}

TEST_CASE("zeros_like inherits dtype by default", "[factories]") {
  auto a = pluss::zeros({2,3}, pluss::ScalarType::Int16);
  auto z = pluss::zeros_like(a);
  REQUIRE(z.scalar_type() == pluss::ScalarType::Int16);
}
