#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::AcfNorm;
using pluss::subsp::stats::ts::AcfOptions;
using pluss::subsp::stats::ts::LjungBoxOptions;
using pluss::subsp::stats::ts::PacfOptions;

Series make_ar1(std::int64_t n, double phi, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::series::randn({n}, cfg, seed).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = phi * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

} // namespace

TEST_CASE("ts diagnostics: constant series is stable", "[ts][diagnostics][edge]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::series::ones({128}, cfg).mul(5.0);

  AcfOptions aopt;
  aopt.max_lag = 12;
  const auto a = pluss::subsp::stats::ts::acf(y, aopt, cfg).to_vec<double>();
  REQUIRE(a.size() == 13);
  REQUIRE(a[0] == Approx(1.0));
  REQUIRE(std::all_of(a.begin() + 1, a.end(), [](double v) { return std::abs(v) <= 1e-12; }));

  PacfOptions popt;
  popt.max_lag = 12;
  const Series p = pluss::subsp::stats::ts::pacf(y, popt, cfg);
  REQUIRE(p.isfinite().all().item<double>() == Approx(1.0));
}

TEST_CASE("ts diagnostics: numerical stability and sensitivity", "[ts][diagnostics][numerics][sensitivity]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series eps = pluss::series::randn({5000}, cfg, 20260314ULL).mul(1e-8).add(1.0);

  AcfOptions biased;
  biased.max_lag = 15;
  biased.norm = AcfNorm::Biased;
  AcfOptions unbiased = biased;
  unbiased.norm = AcfNorm::Unbiased;

  const Series a_b = pluss::subsp::stats::ts::acf(eps, biased, cfg);
  const Series a_u = pluss::subsp::stats::ts::acf(eps, unbiased, cfg);

  REQUIRE(a_b.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(a_u.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(a_b.select(0, 0).item<double>() == Approx(1.0));
  REQUIRE(a_u.select(0, 0).item<double>() == Approx(1.0));
}

TEST_CASE("ts diagnostics: ljung-box distinguishes correlated process", "[ts][diagnostics][accuracy]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series wn = pluss::series::randn({4000}, cfg, 20260315ULL);
  const Series ar = make_ar1(4000, 0.75, 20260316ULL);

  LjungBoxOptions opt;
  opt.lags = {20};

  const auto lb_wn = pluss::subsp::stats::ts::ljung_box(wn, opt, cfg);
  const auto lb_ar = pluss::subsp::stats::ts::ljung_box(ar, opt, cfg);

  REQUIRE(lb_wn.pvalue.item<double>() > 1e-3);
  REQUIRE(lb_ar.pvalue.item<double>() < 1e-10);
}

TEST_CASE("ts diagnostics: stress and crash checks", "[ts][diagnostics][stress][crash]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::series::randn({120000}, cfg, 20260317ULL);

  AcfOptions aopt;
  aopt.max_lag = 40;
  const Series a = pluss::subsp::stats::ts::acf(y, aopt, cfg);
  REQUIRE(a.sizes()[0] == 41);
  REQUIRE(a.isfinite().all().item<double>() == Approx(1.0));

  PacfOptions popt;
  popt.max_lag = 40;
  const Series p = pluss::subsp::stats::ts::pacf(y, popt, cfg);
  REQUIRE(p.sizes()[0] == 41);
  REQUIRE(p.isfinite().all().item<double>() == Approx(1.0));

  LjungBoxOptions lopt;
  lopt.lags = {10, 20, 40};
  const auto lb = pluss::subsp::stats::ts::ljung_box(y, lopt, cfg);
  REQUIRE(lb.stat.sizes()[0] == 3);
  REQUIRE(lb.pvalue.sizes()[0] == 3);
  REQUIRE(lb.stat.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(lb.pvalue.isfinite().all().item<double>() == Approx(1.0));

  AcfOptions bad_acf;
  bad_acf.max_lag = -1;
  REQUIRE_THROWS(pluss::subsp::stats::ts::acf(y, bad_acf, cfg));

  LjungBoxOptions bad_lb;
  bad_lb.lags = {0, 5};
  REQUIRE_THROWS(pluss::subsp::stats::ts::ljung_box(y, bad_lb, cfg));
}

TEST_CASE("ljung-box p-values stay accurate far in the tail", "[ts][ljung_box]") {
  namespace ts = pluss::subsp::stats::ts;
  std::vector<double> x(200);
  double carry = 0.0;
  for (int i = 0; i < 200; ++i) {
    const double t = i + 1.0;
    double u = std::sin(12.9898 * t + 78.233) * 43758.5453;
    u -= std::floor(u);
    carry = 0.7 * carry + (u - 0.5);
    x[static_cast<std::size_t>(i)] = carry;
  }
  ts::LjungBoxOptions options;
  options.lags = {5, 10};
  const auto lb = ts::ljung_box(pluss::make_series(x, {200}).to(pluss::ScalarType::Float64), options);
  const auto stat = lb.stat.to_vec<double>();
  const auto p = lb.pvalue.to_vec<double>();
  // a p-value computed as 1 - cdf would be exactly 0 here
  REQUIRE(stat[0] == Catch::Approx(200.155711).epsilon(1e-6));
  REQUIRE(p[0] == Catch::Approx(2.6308937e-41).epsilon(1e-5));
  REQUIRE(p[1] == Catch::Approx(7.966146e-47).epsilon(1e-5));
}
