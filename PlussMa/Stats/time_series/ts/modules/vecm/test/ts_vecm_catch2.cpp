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
  fit.impact = fit.beta.matmul(fit.alpha.transpose(0, 1));
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

namespace {
// x: random walk; y = x + AR(1); w: an unrelated random walk (400 rows), shocks from a fixed pseudo-random sequence.
pluss::Series cointegrated_triple() {
  const auto shock = [](int i, int offset) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    return (u - std::floor(u)) - 0.5;
  };
  std::vector<double> y(1200);
  double x = 0.0, u = 0.0, w = 0.0;
  for (int t = 0; t < 400; ++t) {
    x += shock(t, 0);
    u = 0.6 * u + shock(t, 5000);
    w += shock(t, 9000);
    y[static_cast<std::size_t>(3 * t)] = x;
    y[static_cast<std::size_t>(3 * t + 1)] = x + u;
    y[static_cast<std::size_t>(3 * t + 2)] = w;
  }
  return pluss::make_series(y, {400, 3}).to(ScalarType::Float64);
}
}  // namespace

// Reference values: an independent implementation of Johansen's procedure (unrestricted constant, no lagged differences).
TEST_CASE("ts vecm: Johansen test and reduced-rank fit match reference values", "[ts][vecm][reference]") {
  const auto y = cointegrated_triple();
  JohansenOptions jopt;
  jopt.lag_order = 1;
  const auto j = johansen_test(y, jopt);
  const auto eig = j.eigenvalues.host<double>();
  const auto stat = j.stat.host<double>();
  const auto p = j.pvalue.host<double>();
  REQUIRE(eig[0] == Approx(0.230144).epsilon(1e-4));
  REQUIRE(eig[1] == Approx(0.026096).epsilon(1e-4));
  REQUIRE(stat[0] == Approx(115.473).epsilon(1e-3));
  REQUIRE(stat[1] == Approx(11.1135).epsilon(1e-3));
  // The limiting law of the trace statistic is not chi-squared: at 3 - r = 2 series the 5% point is about 15.5 (chi-squared with 4 degrees of freedom: 9.5).
  REQUIRE(j.critical.host<double>()[1] == Approx(15.49).epsilon(0.02));
  REQUIRE(p[0] < 1e-6);
  REQUIRE(p[1] > 0.15);
  REQUIRE(p[1] < 0.25);
  REQUIRE(j.selected_rank == 1);

  VecmOptions opt;
  opt.lag_order = 1;
  opt.rank = 1;
  const auto fit = Vecm::fit_mle(y, opt);
  const auto beta = fit.beta.reshape({3}).host<double>();
  const auto alpha = fit.alpha.reshape({3}).host<double>();
  REQUIRE(beta[0] == Approx(1.0));
  REQUIRE(beta[1] == Approx(-0.978829).epsilon(1e-4));
  REQUIRE(beta[2] == Approx(-0.003581).epsilon(2e-3));
  REQUIRE(alpha[0] == Approx(0.028401).epsilon(1e-3));
  REQUIRE(alpha[1] == Approx(0.486002).epsilon(1e-4));
  REQUIRE(fit.loglik(y) == Approx(-193.0305).margin(1e-3));
  // impact = beta alpha'
  REQUIRE(fit.impact.host<double>()[1] == Approx(beta[0] * alpha[1]).epsilon(1e-9));
}
