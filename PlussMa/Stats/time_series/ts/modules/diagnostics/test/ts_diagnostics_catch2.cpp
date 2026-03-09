#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stat::ts::AcfNorm;
using pluss::subsp::stat::ts::AcfOptions;
using pluss::subsp::stat::ts::LjungBoxOptions;
using pluss::subsp::stat::ts::PacfOptions;

Series make_ar1(std::int64_t n, double phi, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::randn({n}, cfg, seed).to_vec<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = phi * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

} // namespace

TEST_CASE("ts diagnostics: constant series is stable", "[ts][diagnostics][edge]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::ones({128}, cfg).mul(5.0);

  AcfOptions aopt;
  aopt.max_lag = 12;
  const auto a = pluss::subsp::stat::ts::acf(y, aopt, cfg).to_vec<double>();
  REQUIRE(a.size() == 13);
  REQUIRE(a[0] == Approx(1.0));
  REQUIRE(std::all_of(a.begin() + 1, a.end(), [](double v) { return std::abs(v) <= 1e-12; }));

  PacfOptions popt;
  popt.max_lag = 12;
  const Series p = pluss::subsp::stat::ts::pacf(y, popt, cfg);
  REQUIRE(p.isfinite().all().item<double>() == Approx(1.0));
}

TEST_CASE("ts diagnostics: numerical stability and sensitivity", "[ts][diagnostics][numerics][sensitivity]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series eps = pluss::randn({5000}, cfg, 20260314ULL).mul(1e-8).add(1.0);

  AcfOptions biased;
  biased.max_lag = 15;
  biased.norm = AcfNorm::Biased;
  AcfOptions unbiased = biased;
  unbiased.norm = AcfNorm::Unbiased;

  const Series a_b = pluss::subsp::stat::ts::acf(eps, biased, cfg);
  const Series a_u = pluss::subsp::stat::ts::acf(eps, unbiased, cfg);

  REQUIRE(a_b.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(a_u.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(a_b.select(0, 0).item<double>() == Approx(1.0));
  REQUIRE(a_u.select(0, 0).item<double>() == Approx(1.0));
}

TEST_CASE("ts diagnostics: ljung-box distinguishes correlated process", "[ts][diagnostics][accuracy]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series wn = pluss::randn({4000}, cfg, 20260315ULL);
  const Series ar = make_ar1(4000, 0.75, 20260316ULL);

  LjungBoxOptions opt;
  opt.lags = {20};

  const auto lb_wn = pluss::subsp::stat::ts::ljung_box(wn, opt, cfg);
  const auto lb_ar = pluss::subsp::stat::ts::ljung_box(ar, opt, cfg);

  REQUIRE(lb_wn.pvalue.item<double>() > 1e-3);
  REQUIRE(lb_ar.pvalue.item<double>() < 1e-10);
}

TEST_CASE("ts diagnostics: stress and crash checks", "[ts][diagnostics][stress][crash]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::randn({120000}, cfg, 20260317ULL);

  AcfOptions aopt;
  aopt.max_lag = 40;
  const Series a = pluss::subsp::stat::ts::acf(y, aopt, cfg);
  REQUIRE(a.sizes()[0] == 41);
  REQUIRE(a.isfinite().all().item<double>() == Approx(1.0));

  PacfOptions popt;
  popt.max_lag = 40;
  const Series p = pluss::subsp::stat::ts::pacf(y, popt, cfg);
  REQUIRE(p.sizes()[0] == 41);
  REQUIRE(p.isfinite().all().item<double>() == Approx(1.0));

  LjungBoxOptions lopt;
  lopt.lags = {10, 20, 40};
  const auto lb = pluss::subsp::stat::ts::ljung_box(y, lopt, cfg);
  REQUIRE(lb.stat.sizes()[0] == 3);
  REQUIRE(lb.pvalue.sizes()[0] == 3);
  REQUIRE(lb.stat.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(lb.pvalue.isfinite().all().item<double>() == Approx(1.0));

  AcfOptions bad_acf;
  bad_acf.max_lag = -1;
  REQUIRE_THROWS(pluss::subsp::stat::ts::acf(y, bad_acf, cfg));

  LjungBoxOptions bad_lb;
  bad_lb.lags = {0, 5};
  REQUIRE_THROWS(pluss::subsp::stat::ts::ljung_box(y, bad_lb, cfg));
}
