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
using pluss::subsp::math::finance::JointLikelihoodWeights;
using pluss::subsp::math::finance::returns_metrics;
using pluss::subsp::math::finance::variance_forecast;
using pluss::subsp::math::finance::weighted_joint_loglik;
using pluss::subsp::math::finance::weighted_joint_nll;
using pluss::subsp::stats::ts::EgarchFit;
using pluss::subsp::stats::ts::GarchFit;
using pluss::subsp::stats::ts::GjrFit;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_garch_bridge() {
  const auto cfg = f64_cpu();
  const GarchFit fit{
      .mean = 0.0,
      .omega = 0.02,
      .arch = {0.08},
      .garch = {0.90},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const Series y = fit.simulate(2000, cfg, 20260430ULL);
  const auto m = returns_metrics(y, fit, cfg);

  expect(std::isfinite(m.loglik), "garch bridge: finite loglik expected");
  expect(std::abs(m.loglik - fit.loglik(y)) < 1e-9, "garch bridge: loglik mismatch");
  expect(m.loglik_per_obs.sizes()[0] == y.numel(), "garch bridge: loglik_per_obs shape mismatch");
  expect(m.variance.sizes()[0] == y.numel(), "garch bridge: variance shape mismatch");
  expect(m.variance.min().item<double>() > 0.0, "garch bridge: variance must be positive");

  const Series f = variance_forecast(12, y, fit, cfg);
  expect(f.sizes()[0] == 12, "garch bridge: forecast shape mismatch");
  expect(f.min().item<double>() > 0.0, "garch bridge: forecast must be positive");
}

void test_egarch_gjr_bridge() {
  const auto cfg = f64_cpu();

  const EgarchFit efit{
      .mean = 0.0,
      .omega = -0.15,
      .arch = {0.12},
      .asym = {-0.06},
      .garch = {0.90},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series ye = efit.simulate(1200, cfg, 20260501ULL);
  const auto em = returns_metrics(ye, efit, cfg);
  expect(std::isfinite(em.loglik), "egarch bridge: finite loglik expected");
  expect(em.variance.min().item<double>() > 0.0, "egarch bridge: variance must be positive");

  const GjrFit gfit{
      .mean = 0.0,
      .omega = 0.02,
      .arch = {0.06},
      .asym = {0.08},
      .garch = {0.86},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };
  const Series yg = gfit.simulate(1200, cfg, 20260502ULL);
  const auto gm = returns_metrics(yg, gfit, cfg);
  expect(std::isfinite(gm.loglik), "gjr bridge: finite loglik expected");
  expect(gm.variance.min().item<double>() > 0.0, "gjr bridge: variance must be positive");
}

void test_joint_likelihood() {
  const JointLikelihoodWeights w{.returns_weight = 0.7, .options_weight = 0.3};
  const double j = weighted_joint_loglik(-120.0, -80.0, w);
  const double nll = weighted_joint_nll(-120.0, -80.0, w);

  expect(std::abs(j - (-108.0)) < 1e-12, "joint loglik mismatch");
  expect(std::abs(nll - 108.0) < 1e-12, "joint nll mismatch");

  bool threw = false;
  try {
    const JointLikelihoodWeights bad{.returns_weight = -1.0, .options_weight = 1.0};
    (void)weighted_joint_loglik(-10.0, -10.0, bad);
  } catch (...) {
    threw = true;
  }
  expect(threw, "joint loglik should throw on negative weights");
}

} // namespace

int main() {
  try {
    test_garch_bridge();
    test_egarch_gjr_bridge();
    test_joint_likelihood();
    std::cout << "vol_bridge_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "vol_bridge_test failed: " << e.what() << '\n';
    return 1;
  }
}
