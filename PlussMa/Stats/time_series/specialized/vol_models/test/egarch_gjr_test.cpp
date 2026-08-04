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
using pluss::subsp::stats::ts::Egarch;
using pluss::subsp::stats::ts::EgarchFit;
using pluss::subsp::stats::ts::EgarchOptions;
using pluss::subsp::stats::ts::Gjr;
using pluss::subsp::stats::ts::GjrFit;
using pluss::subsp::stats::ts::GjrOptions;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series simulate_egarch_truth(std::int64_t n, std::uint64_t seed) {
  const EgarchFit truth{
      .mean = 0.0,
      .omega = -0.15,
      .arch = {0.12},
      .asym = {-0.06},
      .garch = {0.90},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, f64_cpu(), seed);
}

Series simulate_gjr_truth(std::int64_t n, std::uint64_t seed) {
  const GjrFit truth{
      .mean = 0.0,
      .omega = 0.02,
      .arch = {0.06},
      .asym = {0.08},
      .garch = {0.86},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  return truth.simulate(n, f64_cpu(), seed);
}

void test_egarch_slice() {
  const auto cfg = f64_cpu();
  const Series y = simulate_egarch_truth(4500, 20260416ULL);

  EgarchOptions opt;
  opt.arch_order = 1;
  opt.garch_order = 1;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;

  const auto fit = Egarch::fit_mle(y, opt, cfg);
  const Series inf = fit.infer(y, cfg);
  const Series sc = fit.score(y, cfg);
  const Series obs = fit.observed_inf(y, cfg);

  expect(fit.arch.size() == 1 && fit.asym.size() == 1 && fit.garch.size() == 1,
         "egarch fit: coefficient size mismatch");
  expect(fit.persistence() < 1.0, "egarch fit: persistence must be < 1");
  expect(inf.isfinite().all().item<double>() == 1.0, "egarch infer must be finite");
  expect(sc.isfinite().all().item<double>() == 1.0, "egarch score must be finite");
  expect(obs.isfinite().all().item<double>() == 1.0, "egarch observed_inf must be finite");
}

void test_gjr_slice() {
  const auto cfg = f64_cpu();
  const Series y = simulate_gjr_truth(4500, 20260417ULL);

  GjrOptions opt;
  opt.arch_order = 1;
  opt.garch_order = 1;
  opt.max_iterations = 1200;
  opt.max_evaluations = 12000;

  const auto fit = Gjr::fit_mle(y, opt, cfg);
  const Series inf = fit.infer(y, cfg);
  const Series f = fit.forecast(12, y, cfg);

  expect(fit.arch.size() == 1 && fit.asym.size() == 1 && fit.garch.size() == 1,
         "gjr fit: coefficient size mismatch");
  expect(fit.persistence() < 1.0, "gjr fit: persistence must be < 1");
  expect(inf.min().item<double>() > 0.0, "gjr infer must be positive");
  expect(f.min().item<double>() > 0.0, "gjr forecast must be positive");
}

void test_error_paths() {
  const auto cfg = f64_cpu();
  const Series y = pluss::series::randn({300}, cfg, 20260418ULL);

  bool threw = false;
  try {
    EgarchOptions bad;
    bad.stationarity_margin = 1.0;
    (void)Egarch::fit_mle(y, bad, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "egarch fit should throw for invalid stationarity_margin");

  threw = false;
  try {
    auto fit = Gjr::fit_mle(y, {}, cfg);
    (void)fit.forecast(0, y, cfg);
  } catch (...) {
    threw = true;
  }
  expect(threw, "gjr forecast should throw for non-positive horizon");
}

} // namespace

int main() {
  try {
    test_egarch_slice();
    test_gjr_slice();
    test_error_paths();
    std::cout << "egarch_gjr_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "egarch_gjr_test failed: " << e.what() << '\n';
    return 1;
  }
}
