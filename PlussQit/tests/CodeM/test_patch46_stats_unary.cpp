#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
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

TEST_CASE("root order statistics validate ranks and missing values", "[stats]") {
  auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  REQUIRE(values.median().item<double>() == 2.5);
  REQUIRE(values.quantile(0.25).item<double>() == 1.75);
  REQUIRE_THROWS(values.quantile(-0.1));
  REQUIRE_THROWS(values.nanquantile(1.1));

  auto missing = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 5.0});
  REQUIRE(std::isnan(missing.median().item<double>()));
  REQUIRE(missing.nanmedian().item<double>() == 3.0);
  REQUIRE(missing.nanquantile(0.5).item<double>() == 3.0);

  auto empty = pluss::series::zeros({0});
  REQUIRE_THROWS(empty.mode());

  auto empty_lane = pluss::series::zeros({2, 0});
  REQUIRE_THROWS(empty_lane.mode(1));
}
