#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("codegen method surface includes unary math ops", "[codegen][unary]") {
  pluss::Series x = pluss::make_series(std::vector<float>{0.0f, 1.0f, 2.0f, 3.0f}, {2,2});
  auto s = x.sin();
  auto c = x.cos();
  auto t = x.tanh();
  auto r = x.reciprocal();
  REQUIRE(s.numel() == 4);
  REQUIRE(c.numel() == 4);
  REQUIRE(t.numel() == 4);
  REQUIRE(r.numel() == 4);

  auto lf = x.log1p();
  auto ef = x.expm1();
  REQUIRE(lf.numel() == 4);
  REQUIRE(ef.numel() == 4);

  auto m = x.isfinite();
  REQUIRE(m.scalar_type() == pluss::ScalarType::Bool);

  auto ln = x.logical_not();
  REQUIRE(ln.scalar_type() == pluss::ScalarType::Bool);

  auto x_inplace = x.clone();
  x_inplace.logical_not_();
  REQUIRE(x_inplace.scalar_type() == pluss::ScalarType::Bool);
  REQUIRE(x_inplace.host<std::uint8_t>() == std::vector<std::uint8_t>{1, 0, 0, 0});
}
