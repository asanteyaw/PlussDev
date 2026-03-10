#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::stats::ts::Garch11;
using pluss::subsp::stats::ts::Garch11Fit;
using pluss::subsp::stats::ts::Garch11Options;
using pluss::subsp::stats::ts::Garch;
using pluss::subsp::stats::ts::GarchFit;
using pluss::subsp::stats::ts::GarchOptions;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_fit_recovery() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Garch11Fit truth{
      .mean = 0.0,
      .omega = 0.02,
      .alpha = 0.08,
      .beta = 0.90,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series y = truth.simulate(7000, cfg, 20260320ULL);

  Garch11Options opt;
  opt.demean = true;
  opt.omega0 = 0.03;
  opt.alpha0 = 0.10;
  opt.beta0 = 0.85;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Garch11::fit_mle(y, opt, cfg);

  expect(std::abs(fit.alpha - truth.alpha) < 0.09, "alpha recovery too far");
  expect(std::abs(fit.beta - truth.beta) < 0.12, "beta recovery too far");
  expect(std::abs((fit.alpha + fit.beta) - (truth.alpha + truth.beta)) < 0.08, "persistence recovery too far");
}

void test_methods_finite_and_shapes() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Garch11Fit truth{
      .mean = 0.0,
      .omega = 0.03,
      .alpha = 0.10,
      .beta = 0.85,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series y = truth.simulate(2400, cfg, 20260321ULL);
  const auto fit = Garch11::fit_mle(y, {}, cfg);

  const Series inf = fit.infer(y, cfg);
  const Series llobs = fit.loglik_per_obs(y, cfg);
  const Series sc = fit.score(y, cfg);
  const Series obs = fit.observed_inf(y, cfg);
  const Series fc = fit.forecast(12, y, cfg);

  expect(inf.sizes().size() == 1 && inf.sizes()[0] == y.numel(), "infer shape mismatch");
  expect(llobs.sizes().size() == 1 && llobs.sizes()[0] == y.numel(), "loglik_per_obs shape mismatch");
  expect(sc.sizes().size() == 1 && sc.sizes()[0] == 3, "score shape mismatch");
  expect(obs.sizes().size() == 2 && obs.sizes()[0] == 3 && obs.sizes()[1] == 3, "observed_inf shape mismatch");
  expect(fc.sizes().size() == 1 && fc.sizes()[0] == 12, "forecast shape mismatch");

  expect(inf.isfinite().all().item<double>() == 1.0, "infer must be finite");
  expect(llobs.isfinite().all().item<double>() == 1.0, "loglik_per_obs must be finite");
  expect(sc.isfinite().all().item<double>() == 1.0, "score must be finite");
  expect(obs.isfinite().all().item<double>() == 1.0, "observed_inf must be finite");
  expect(fc.isfinite().all().item<double>() == 1.0, "forecast must be finite");
  expect(inf.min().item<double>() > 0.0, "inferred variance must be positive");
}

void test_error_paths() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const Series y = pluss::randn({400}, cfg, 20260322ULL);
  const auto fit = Garch11::fit_mle(y, {}, cfg);

  bool threw = false;
  try {
    (void)fit.forecast(0, y, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "forecast should throw for non-positive horizon");

  threw = false;
  try {
    Garch11Options bad;
    bad.variance_floor = -1.0;
    (void)Garch11::fit_mle(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "fit_mle should throw for invalid options");
}

void test_garch_pq_slice() {
  const auto cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const GarchFit truth{
      .mean = 0.0,
      .omega = 0.015,
      .arch = {0.06, 0.04},
      .garch = {0.80},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series y = truth.simulate(5500, cfg, 20260329ULL);

  GarchOptions opt;
  opt.arch_order = 2;
  opt.garch_order = 1;
  opt.omega0 = 0.02;
  opt.arch0 = {0.05, 0.05};
  opt.garch0 = {0.85};
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;
  const auto fit = Garch::fit_mle(y, opt, cfg);

  expect(fit.arch.size() == 2, "garch(p,q): arch size mismatch");
  expect(fit.garch.size() == 1, "garch(p,q): garch size mismatch");
  expect(fit.persistence() < 1.0, "garch(p,q): persistence must be < 1");
  expect(fit.forecast(10, y, cfg).isfinite().all().item<double>() == 1.0, "garch(p,q): finite forecast expected");
}

} // namespace

int main() {
  try {
    test_fit_recovery();
    test_methods_finite_and_shapes();
    test_error_paths();
    test_garch_pq_slice();
    std::cout << "garch11_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "garch11_test failed: " << e.what() << '\n';
    return 1;
  }
}
