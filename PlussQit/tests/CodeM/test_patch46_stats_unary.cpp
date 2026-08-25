#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <limits>

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

  auto singleton_lanes = pluss::make_series(std::vector<float>{1, 2}, {2, 1});
  REQUIRE_THROWS(singleton_lanes.var(1, true));
}

TEST_CASE("stat accessor matches dimensional statistics surface", "[stats]") {
  auto x = pluss::make_series(std::vector<float>{1, 2, 3, 4}, {2, 2});
  auto with_nan = pluss::make_series(
      std::vector<float>{1, std::numeric_limits<float>::quiet_NaN(), 3, 5}, {2, 2});

  REQUIRE(x.stat.var(1, false, true).sizes() == std::vector<std::int64_t>{2, 1});
  REQUIRE(with_nan.stat.nanvar(1, true).sizes() == std::vector<std::int64_t>{2, 1});
  REQUIRE(with_nan.stat.nanstd(1).sizes() == std::vector<std::int64_t>{2});
  REQUIRE(x.stat.skewness(1).sizes() == std::vector<std::int64_t>{2});
  REQUIRE(x.stat.kurtosis(1).sizes() == std::vector<std::int64_t>{2});
  REQUIRE(x.stat.center(1).sizes() == x.sizes());
  REQUIRE(x.stat.standardize(1).sizes() == x.sizes());
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
