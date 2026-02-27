#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("var/std run", "[stats]") {
  auto x = pluss::make_series(std::vector<float>{1,2,3,4}, {2,2});
  auto v = x.var(false);
  REQUIRE(v.numel() == 1);
  auto s = x.std(false);
  REQUIRE(s.numel() == 1);

  auto v0 = x.var(1, false, false);
  REQUIRE(v0.sizes().size() == 1);
  REQUIRE(v0.sizes()[0] == 2);

  auto s0 = x.std(1, false, true);
  REQUIRE(s0.sizes().size() == 2);
  REQUIRE(s0.sizes()[1] == 1);
}

TEST_CASE("exp/log/sqrt run", "[unary]") {
  auto x = pluss::make_series(std::vector<float>{1.0f, 2.0f, 4.0f}, {3});
  auto e = x.exp();
  auto l = x.log();
  auto r = x.sqrt();
  REQUIRE(e.numel() == 3);
  REQUIRE(l.numel() == 3);
  REQUIRE(r.numel() == 3);
}
