#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Config;
using pluss::ScalarType;
using pluss::subsp::stats::ts::JohansenOptions;
using pluss::subsp::stats::ts::JohansenStat;
using pluss::subsp::stats::ts::Vecm;
using pluss::subsp::stats::ts::VecmFit;
using pluss::subsp::stats::ts::VecmOptions;
using pluss::subsp::stats::ts::johansen_test;

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

VecmFit vecm_truth() {
  const auto cfg = f64_cpu();
  VecmFit fit;
  fit.lag_order = 2;
  fit.rank = 1;
  fit.include_intercept = true;
  fit.include_trend = false;
  fit.alpha = pluss::make_series(std::vector<double>{-0.55, 0.42}, {2, 1}, cfg);
  fit.beta = pluss::make_series(std::vector<double>{1.0, -1.0}, {2, 1}, cfg);
  fit.impact = fit.alpha.matmul(fit.beta.transpose(0, 1));
  fit.gamma = pluss::make_series(
      std::vector<double>{
          0.08, 0.01,
         -0.03, 0.05,
      },
      {1, 2, 2},
      cfg);
  fit.constant = pluss::make_series(std::vector<double>{0.012, -0.007}, {2}, cfg);
  fit.trend = pluss::series::zeros({2}, cfg);
  fit.covariance = pluss::make_series(std::vector<double>{0.22, 0.03, 0.03, 0.18}, {2, 2}, cfg);
  fit.sample_size = 0;
  return fit;
}

} // namespace

TEST_CASE("ts vecm: johansen rank + fit/infer/loglik are finite", "[ts][vecm][fit]") {
  const auto cfg = f64_cpu();
  const VecmFit truth = vecm_truth();
  const auto y = truth.simulate(4200, cfg, 20260320ULL);

  JohansenOptions jopt;
  jopt.lag_order = 2;
  jopt.include_intercept = true;
  jopt.include_trend = false;
  jopt.statistic = JohansenStat::Trace;
  jopt.alpha = 0.05;

  const auto jres = johansen_test(y, jopt, cfg);
  REQUIRE(jres.eigenvalues.sizes()[0] == 2);
  REQUIRE(jres.stat.sizes()[0] == 2);
  REQUIRE(jres.pvalue.sizes()[0] == 2);
  REQUIRE(jres.stat.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(jres.pvalue.min().item<double>() >= 0.0);
  REQUIRE(jres.pvalue.max().item<double>() <= 1.0);
  REQUIRE(jres.selected_rank >= 1);

  VecmOptions opt;
  opt.lag_order = 2;
  opt.rank = -1;
  opt.include_intercept = true;
  opt.include_trend = false;
  opt.alpha = 0.05;
  opt.ridge = 1e-8;
  opt.pinv_rcond = 1e-12;

  const auto fit = Vecm::fit_mle(y, opt, cfg);
  const auto resid = fit.infer(y, cfg);
  const auto llobs = fit.loglik_per_obs(y, cfg);

  REQUIRE(fit.rank == jres.selected_rank);
  REQUIRE(fit.impact.sizes()[0] == 2);
  REQUIRE(fit.impact.sizes()[1] == 2);
  REQUIRE(fit.gamma.sizes().size() == 3);
  REQUIRE(fit.gamma.sizes()[0] == 1);
  REQUIRE(fit.gamma.sizes()[1] == 2);
  REQUIRE(fit.gamma.sizes()[2] == 2);
  REQUIRE(fit.covariance.sizes()[0] == 2);
  REQUIRE(fit.covariance.sizes()[1] == 2);

  REQUIRE(resid.sizes()[0] == y.sizes()[0] - 2);
  REQUIRE(resid.sizes()[1] == 2);
  REQUIRE(resid.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(llobs.sizes()[0] == y.sizes()[0] - 2);
  REQUIRE(llobs.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(std::isfinite(fit.loglik(y)));
}

TEST_CASE("ts vecm: forecast/simulate shapes and finite outputs", "[ts][vecm][forecast]") {
  const auto cfg = f64_cpu();
  const VecmFit truth = vecm_truth();
  const auto y = truth.simulate(1600, cfg, 20260321ULL);

  VecmOptions opt;
  opt.lag_order = 2;
  opt.rank = 1;
  opt.include_intercept = true;
  opt.include_trend = false;
  opt.ridge = 1e-8;
  opt.pinv_rcond = 1e-12;

  const auto fit = Vecm::fit_mle(y, opt, cfg);

  const auto f = fit.forecast(14, y, cfg);
  REQUIRE(f.sizes().size() == 2);
  REQUIRE(f.sizes()[0] == 14);
  REQUIRE(f.sizes()[1] == 2);
  REQUIRE(f.isfinite().all().item<double>() == Approx(1.0));

  const auto sim = fit.simulate(320, cfg, 20260322ULL);
  REQUIRE(sim.sizes().size() == 2);
  REQUIRE(sim.sizes()[0] == 320);
  REQUIRE(sim.sizes()[1] == 2);
  REQUIRE(sim.isfinite().all().item<double>() == Approx(1.0));

  REQUIRE_THROWS(fit.forecast(0, y, cfg));
  REQUIRE_THROWS(fit.simulate(2, cfg, 20260323ULL));
}
