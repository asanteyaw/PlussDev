#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <pluss/pl/studio.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::AcfNorm;

Series make_ar1(std::int64_t n, double phi, std::uint64_t seed) {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto eps = pluss::series::randn({n}, cfg, seed).host<double>();
  std::vector<double> y(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t t = 1; t < n; ++t) {
    y[static_cast<std::size_t>(t)] = phi * y[static_cast<std::size_t>(t - 1)] + eps[static_cast<std::size_t>(t)];
  }
  return pluss::make_series(y, {n}, cfg);
}

} // namespace

TEST_CASE("ts plotting: payload numerics are finite", "[ts][plot][payload]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = make_ar1(4096, 0.65, 20260408ULL);

  pluss::subsp::stats::ts::plot::CorrPlotOptions opt;
  opt.max_lag = 24;
  opt.norm = AcfNorm::Biased;
  opt.include_zero_lag = false;

  const auto a = pluss::subsp::stats::ts::plot::acf_payload(y, opt, cfg);
  const auto p = pluss::subsp::stats::ts::plot::pacf_payload(y, opt, cfg);

  REQUIRE(a.values.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(p.values.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(a.lags.sizes()[0] == 24);
  REQUIRE(p.lags.sizes()[0] == 24);
  REQUIRE(a.conf_high > 0.0);
  REQUIRE(p.conf_high > 0.0);
}

TEST_CASE("ts plotting: residual payload keeps length", "[ts][plot][residual]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::series::randn({1500}, cfg, 20260409ULL);
  const auto payload = pluss::subsp::stats::ts::plot::residuals_payload(y, cfg);

  REQUIRE(payload.index.numel() == y.numel());
  REQUIRE(payload.residuals.numel() == y.numel());
  REQUIRE(payload.residuals.isfinite().all().item<double>() == Approx(1.0));
}

TEST_CASE("ts plotting: flash rendering hooks are callable", "[ts][plot][flash]") {
  const auto cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = make_ar1(1024, 0.7, 20260410ULL);

  pluss::subsp::stats::ts::plot::CorrPlotOptions corr_opt;
  corr_opt.max_lag = 16;
  corr_opt.include_zero_lag = true;

  pluss::subsp::stats::ts::plot::ResidualPlotOptions resid_opt;

  pluss::pl::Studio acf_fig("ts-plot-catch2-acf", pluss::pl::Theme::paper());
  REQUIRE_NOTHROW(pluss::subsp::stats::ts::plot::acf(acf_fig, y, corr_opt, cfg));

  pluss::pl::Studio pacf_fig("ts-plot-catch2-pacf", pluss::pl::Theme::paper());
  REQUIRE_NOTHROW(pluss::subsp::stats::ts::plot::pacf(pacf_fig, y, corr_opt, cfg));

  pluss::pl::Studio resid_fig("ts-plot-catch2-resid", pluss::pl::Theme::paper());
  REQUIRE_NOTHROW(pluss::subsp::stats::ts::plot::residuals(resid_fig, y, resid_opt, cfg));
}
