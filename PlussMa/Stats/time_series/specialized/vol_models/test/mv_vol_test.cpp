#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::EgarchFit;
using pluss::subsp::stats::ts::Garch11Fit;
using pluss::subsp::stats::ts::GjrFit;
using pluss::subsp::stats::ts::MvEgarch;
using pluss::subsp::stats::ts::MvEgarchOptions;
using pluss::subsp::stats::ts::MvGarch;
using pluss::subsp::stats::ts::MvGarchOptions;
using pluss::subsp::stats::ts::MvGjr;
using pluss::subsp::stats::ts::MvGjrOptions;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series make_two_series_(const Series& a, const Series& b) {
  const auto n = a.numel();
  return a.reshape({n, 1}).unite({b.reshape({n, 1})}, 1);
}

void check_corr_(const Series& corr, const std::string& name) {
  expect(corr.sizes().size() == 2 && corr.sizes()[0] == corr.sizes()[1],
         name + ": corr must be square");
  expect(corr.isfinite().all().item<double>() == 1.0, name + ": corr must be finite");
  const auto k = corr.sizes()[0];
  const Series eye = pluss::eye(k, f64_cpu());
  const Series diag = corr.mul(eye).sum(1, false);
  const Series diag_err = diag.sub(pluss::ones({k}, f64_cpu())).abs();
  expect(diag_err.max().item<double>() < 1e-3, name + ": corr diagonal should be ~1");
}

void test_mv_garch() {
  const auto cfg = f64_cpu();
  const Garch11Fit g1{
      .mean = 0.0,
      .omega = 0.015,
      .alpha = 0.07,
      .beta = 0.90,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Garch11Fit g2{
      .mean = 0.0,
      .omega = 0.020,
      .alpha = 0.09,
      .beta = 0.86,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const Series y1 = g1.simulate(900, cfg, 20270101ULL);
  const Series y2i = g2.simulate(900, cfg, 20270102ULL);
  const Series y2 = y2i.mul(0.75).add(y1.mul(0.25));
  const Series y = make_two_series_(y1, y2);

  MvGarchOptions opt;
  opt.marginal.max_iterations = 260;
  opt.marginal.max_evaluations = 2600;
  opt.marginal.nm_restarts = 1;
  opt.corr_shrinkage = 1e-4;
  opt.corr_jitter = 1e-9;

  const auto fit = MvGarch::fit_mle(y, opt, cfg);
  const Series inf = fit.infer(y, cfg);
  const Series llobs = fit.loglik_per_obs(y, cfg);
  const Series f = fit.forecast(16, y, cfg);

  expect(static_cast<std::int64_t>(fit.marginals.size()) == 2, "MvGarch: expected 2 marginals");
  expect(inf.sizes().size() == 2 && inf.sizes()[0] == 900 && inf.sizes()[1] == 2,
         "MvGarch: infer shape mismatch");
  expect(llobs.sizes().size() == 1 && llobs.sizes()[0] == 900, "MvGarch: loglik_per_obs shape mismatch");
  expect(f.sizes().size() == 2 && f.sizes()[0] == 16 && f.sizes()[1] == 2,
         "MvGarch: forecast shape mismatch");
  expect(std::isfinite(fit.loglik(y)), "MvGarch: finite loglik expected");
  expect(inf.min().item<double>() > 0.0, "MvGarch: inferred variances must be positive");
  check_corr_(fit.corr, "MvGarch");
}

void test_mv_egarch_gjr() {
  const auto cfg = f64_cpu();
  const EgarchFit e1{
      .mean = 0.0,
      .omega = -0.12,
      .arch = {0.11},
      .asym = {-0.04},
      .garch = {0.89},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const EgarchFit e2{
      .mean = 0.0,
      .omega = -0.08,
      .arch = {0.09},
      .asym = {-0.02},
      .garch = {0.91},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const GjrFit g1{
      .mean = 0.0,
      .omega = 0.015,
      .arch = {0.06},
      .asym = {0.06},
      .garch = {0.87},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const GjrFit g2{
      .mean = 0.0,
      .omega = 0.018,
      .arch = {0.07},
      .asym = {0.05},
      .garch = {0.86},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const Series ye1 = e1.simulate(900, cfg, 20270103ULL);
  const Series ye2 = e2.simulate(900, cfg, 20270104ULL).mul(0.8).add(ye1.mul(0.2));
  const Series yg1 = g1.simulate(900, cfg, 20270105ULL);
  const Series yg2 = g2.simulate(900, cfg, 20270106ULL).mul(0.8).add(yg1.mul(0.2));

  const Series ye = make_two_series_(ye1, ye2);
  const Series yg = make_two_series_(yg1, yg2);

  MvEgarchOptions eopt;
  eopt.marginal.max_iterations = 220;
  eopt.marginal.max_evaluations = 2400;
  eopt.marginal.nm_restarts = 1;
  eopt.corr_shrinkage = 1e-4;

  MvGjrOptions gopt;
  gopt.marginal.max_iterations = 220;
  gopt.marginal.max_evaluations = 2400;
  gopt.marginal.nm_restarts = 1;
  gopt.corr_shrinkage = 1e-4;

  const auto efit = MvEgarch::fit_mle(ye, eopt, cfg);
  const auto gfit = MvGjr::fit_mle(yg, gopt, cfg);

  expect(std::isfinite(efit.loglik(ye)), "MvEgarch: finite loglik expected");
  expect(std::isfinite(gfit.loglik(yg)), "MvGjr: finite loglik expected");
  expect(efit.infer(ye, cfg).min().item<double>() > 0.0, "MvEgarch: inferred variances must be positive");
  expect(gfit.infer(yg, cfg).min().item<double>() > 0.0, "MvGjr: inferred variances must be positive");
  check_corr_(efit.corr, "MvEgarch");
  check_corr_(gfit.corr, "MvGjr");
}

} // namespace

int main() {
  try {
    test_mv_garch();
    test_mv_egarch_gjr();
    std::cout << "mv_vol_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mv_vol_test failed: " << e.what() << '\n';
    return 1;
  }
}
