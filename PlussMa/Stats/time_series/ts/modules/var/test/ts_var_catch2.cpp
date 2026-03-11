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
using pluss::subsp::stats::ts::Var;
using pluss::subsp::stats::ts::VarFit;
using pluss::subsp::stats::ts::FevdOptions;
using pluss::subsp::stats::ts::GrangerOptions;
using pluss::subsp::stats::ts::GrangerStat;
using pluss::subsp::stats::ts::GrangerType;
using pluss::subsp::stats::ts::IrfMethod;
using pluss::subsp::stats::ts::IrfOptions;
using pluss::subsp::stats::ts::VarOptions;
using pluss::subsp::stats::ts::var_to_vecm;
using pluss::subsp::stats::ts::vecm_to_var;

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

double max_abs_diff(const Series& a, const Series& b) {
  const auto av = a.to(pluss::pCPU, ScalarType::Float64).reshape({a.numel()}).to_vec<double>();
  const auto bv = b.to(pluss::pCPU, ScalarType::Float64).reshape({b.numel()}).to_vec<double>();
  double m = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i) {
    m = std::max(m, std::abs(av[i] - bv[i]));
  }
  return m;
}

VarFit var_truth_no_exog() {
  const auto cfg = f64_cpu();
  VarFit fit;
  fit.lag_order = 2;
  fit.include_intercept = true;
  fit.include_trend = false;
  fit.constant = pluss::make_series(std::vector<double>{0.10, -0.05}, {2}, cfg);
  fit.trend = pluss::zeros({2}, cfg);
  fit.ar = pluss::make_series(
      std::vector<double>{
          0.45, 0.08,
         -0.04, 0.35,
         -0.10, 0.03,
          0.02,-0.15,
      },
      {2, 2, 2},
      cfg);
  fit.beta = pluss::empty({0, 2}, cfg);
  fit.covariance = pluss::make_series(std::vector<double>{0.35, 0.08, 0.08, 0.25}, {2, 2}, cfg);
  return fit;
}

VarFit var_truth_with_exog() {
  const auto cfg = f64_cpu();
  VarFit fit;
  fit.lag_order = 2;
  fit.include_intercept = true;
  fit.include_trend = true;
  fit.constant = pluss::make_series(std::vector<double>{0.05, -0.02}, {2}, cfg);
  fit.trend = pluss::make_series(std::vector<double>{0.0005, -0.0003}, {2}, cfg);
  fit.ar = pluss::make_series(
      std::vector<double>{
          0.50, 0.05,
         -0.03, 0.30,
         -0.12, 0.04,
          0.01,-0.18,
      },
      {2, 2, 2},
      cfg);
  fit.beta = pluss::make_series(
      std::vector<double>{
          0.60,-0.20,
         -0.10, 0.40,
      },
      {2, 2},
      cfg);
  fit.covariance = pluss::make_series(std::vector<double>{0.30, 0.06, 0.06, 0.20}, {2, 2}, cfg);
  return fit;
}

} // namespace

TEST_CASE("ts var: fit/infer/loglik are finite and stable", "[ts][var][fit]") {
  const auto cfg = f64_cpu();
  const auto truth = var_truth_no_exog();
  const Series y = truth.simulate(4500, cfg, 20260601ULL);

  VarOptions opt;
  opt.lag_order = 2;
  opt.include_intercept = true;
  opt.include_trend = false;
  opt.ridge = 1e-8;

  const auto fit = Var::fit_mle(y, opt, cfg);
  const Series inf = fit.infer(y, cfg);
  const Series llobs = fit.loglik_per_obs(y, cfg);

  REQUIRE(fit.ar.sizes()[0] == 2);
  REQUIRE(fit.ar.sizes()[1] == 2);
  REQUIRE(fit.ar.sizes()[2] == 2);
  REQUIRE(inf.sizes()[0] == y.sizes()[0] - 2);
  REQUIRE(inf.sizes()[1] == 2);
  REQUIRE(inf.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(llobs.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(std::isfinite(fit.loglik(y)));

  const double ar_err = max_abs_diff(fit.ar, truth.ar);
  REQUIRE(ar_err < 0.20);
}

TEST_CASE("ts varx: exogenous fit + forecast path", "[ts][var][varx]") {
  const auto cfg = f64_cpu();
  const auto truth = var_truth_with_exog();
  const Series x = pluss::randn({5000, 2}, cfg, 20260602ULL);
  const Series y = truth.simulate(x, cfg, 20260603ULL);

  VarOptions opt;
  opt.lag_order = 2;
  opt.include_intercept = true;
  opt.include_trend = true;
  opt.ridge = 1e-8;

  const auto fit = Var::fit_mle(y, x, opt, cfg);
  REQUIRE(fit.beta.sizes()[0] == 2);
  REQUIRE(fit.beta.sizes()[1] == 2);
  REQUIRE(fit.infer(y, x, cfg).isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fit.loglik_per_obs(y, x, cfg).isfinite().all().item<double>() == Approx(1.0));

  const Series xf = pluss::randn({16, 2}, cfg, 20260604ULL);
  const Series f = fit.forecast(16, y, xf, cfg);
  REQUIRE(f.sizes()[0] == 16);
  REQUIRE(f.sizes()[1] == 2);
  REQUIRE(f.isfinite().all().item<double>() == Approx(1.0));
}

TEST_CASE("ts var/vec conversion roundtrip + edge checks", "[ts][var][vec]") {
  const auto cfg = f64_cpu();
  const auto truth = var_truth_no_exog();

  const auto conv = var_to_vecm(truth.ar, cfg);
  const Series ar_back = vecm_to_var(conv.first, conv.second, cfg);
  REQUIRE(max_abs_diff(truth.ar, ar_back) < 1e-10);

  VarOptions opt;
  opt.lag_order = 2;
  const Series bad_y = pluss::randn({2, 2}, cfg, 20260605ULL);
  REQUIRE_THROWS(Var::fit_mle(bad_y, opt, cfg));

  const Series y = truth.simulate(200, cfg, 20260606ULL);
  const auto fit = Var::fit_mle(y, opt, cfg);
  REQUIRE_THROWS(fit.forecast(0, y, cfg));
}

TEST_CASE("ts var: irf/fevd structural diagnostics are finite", "[ts][var][structural]") {
  const auto cfg = f64_cpu();
  const auto truth = var_truth_no_exog();
  const Series y = truth.simulate(3200, cfg, 20260621ULL);

  VarOptions opt;
  opt.lag_order = 2;
  opt.include_intercept = true;
  opt.include_trend = false;
  opt.ridge = 1e-8;
  const auto fit = Var::fit_mle(y, opt, cfg);

  IrfOptions iopt;
  iopt.horizon = 18;
  iopt.method = IrfMethod::Orthogonalized;
  const Series irf = fit.irf(iopt, cfg);
  REQUIRE(irf.sizes().size() == 3);
  REQUIRE(irf.sizes()[0] == 18);
  REQUIRE(irf.sizes()[1] == 2);
  REQUIRE(irf.sizes()[2] == 2);
  REQUIRE(irf.isfinite().all().item<double>() == Approx(1.0));

  FevdOptions fopt;
  fopt.horizon = 18;
  fopt.method = IrfMethod::Orthogonalized;
  const Series fevd = fit.fevd(fopt, cfg);
  REQUIRE(fevd.sizes().size() == 3);
  REQUIRE(fevd.sizes()[0] == 18);
  REQUIRE(fevd.sizes()[1] == 2);
  REQUIRE(fevd.sizes()[2] == 2);
  REQUIRE(fevd.isfinite().all().item<double>() == Approx(1.0));
  REQUIRE(fevd.min().item<double>() >= -1e-10);

  const Series row_sum = fevd.sum(1, false);
  const double max_row_error = row_sum.sub(1.0).abs().max().item<double>();
  REQUIRE(max_row_error < 5e-5);
}

TEST_CASE("ts var: granger_test detects directional lag-causality", "[ts][var][granger]") {
  const auto cfg = f64_cpu();
  VarFit truth;
  truth.lag_order = 1;
  truth.include_intercept = false;
  truth.include_trend = false;
  truth.constant = pluss::zeros({2}, cfg);
  truth.trend = pluss::zeros({2}, cfg);
  truth.ar = pluss::make_series(
      std::vector<double>{
          0.20, 0.00,  // source 0 -> eq0,eq1
          0.70, 0.40,  // source 1 -> eq0,eq1
      },
      {1, 2, 2},
      cfg);
  truth.beta = pluss::empty({0, 2}, cfg);
  truth.covariance = pluss::make_series(std::vector<double>{0.25, 0.02, 0.02, 0.20}, {2, 2}, cfg);

  const Series y = truth.simulate(4500, cfg, 20260622ULL);

  VarOptions opt;
  opt.lag_order = 1;
  opt.include_intercept = false;
  opt.include_trend = false;
  opt.ridge = 1e-8;
  const auto fit = Var::fit_mle(y, opt, cfg);

  GrangerOptions gopt;
  gopt.type = GrangerType::LeaveOneOut;
  gopt.stat = GrangerStat::ChiSquare;
  gopt.alpha = 0.05;
  const auto res = fit.granger_test(y, gopt, cfg);

  const auto cause = res.cause.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  const auto effect = res.effect.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  const auto pvals = res.pvalue.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  REQUIRE(cause.size() == effect.size());
  REQUIRE(effect.size() == pvals.size());

  double p_1_to_0 = 1.0;
  double p_0_to_1 = 1.0;
  for (std::size_t i = 0; i < pvals.size(); ++i) {
    if (cause[i] == 1 && effect[i] == 0) {
      p_1_to_0 = pvals[i];
    }
    if (cause[i] == 0 && effect[i] == 1) {
      p_0_to_1 = pvals[i];
    }
  }

  REQUIRE(p_1_to_0 < 1e-6);
  REQUIRE(p_0_to_1 > 1e-3);
}
