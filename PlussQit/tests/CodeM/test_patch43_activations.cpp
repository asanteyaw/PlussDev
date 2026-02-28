#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("activation methods exist and run", "[activations]") {
  auto x = pluss::make_series(std::vector<float>{-2, -1, 0, 1, 2}, {5});
  auto s = x.sigmoid();
  REQUIRE(s.scalar_type() == pluss::ScalarType::Float32);

  auto sp = x.softplus();
  REQUIRE(sp.scalar_type() == pluss::ScalarType::Float32);

  auto si = x.silu();
  REQUIRE(si.scalar_type() == pluss::ScalarType::Float32);

  auto g = x.gelu(true);
  REQUIRE(g.scalar_type() == pluss::ScalarType::Float32);

  auto l = x.leaky_relu(0.2);
  REQUIRE(l.scalar_type() == pluss::ScalarType::Float32);
}
