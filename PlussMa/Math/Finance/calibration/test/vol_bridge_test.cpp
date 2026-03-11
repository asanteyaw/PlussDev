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
using pluss::subsp::math::finance::calibration::JointLikelihoodWeights;
using pluss::subsp::math::finance::calibration::returns_metrics;
using pluss::subsp::math::finance::calibration::variance_forecast;
using pluss::subsp::math::finance::calibration::weighted_joint_loglik;
using pluss::subsp::math::finance::calibration::weighted_joint_nll;
using pluss::subsp::math::finance::pricing::pricing_metrics;
using pluss::subsp::math::finance::pricing::pricing_variance_forecast;
using pluss::subsp::math::finance::volatility::Garch11Params;
using pluss::subsp::math::finance::volatility::Ngarch11Params;
using pluss::subsp::math::finance::volatility::to_pricing_params;
using pluss::subsp::math::finance::volatility::unconditional_variance;
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

void test_ngarch_pricing_side() {
  const auto cfg = f64_cpu();
  const Ngarch11Params np{
      .omega = 0.02,
      .alpha = 0.08,
      .beta = 0.88,
      .gamma = -0.25,
      .lambda = 0.15,
      .r = 0.01,
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
  };

  const Series y = pluss::randn({1800}, cfg, 20260503ULL).mul(0.25);
  const auto nm = pricing_metrics(y, np, cfg);
  expect(std::isfinite(nm.loglik), "ngarch pricing: finite loglik expected");
  expect(nm.loglik_per_obs.sizes()[0] == y.numel(), "ngarch pricing: per-obs shape mismatch");
  expect(nm.variance.sizes()[0] == y.numel(), "ngarch pricing: variance shape mismatch");
  expect(nm.mean.sizes()[0] == y.numel(), "ngarch pricing: mean shape mismatch");
  expect(nm.standardized_innovation.sizes()[0] == y.numel(), "ngarch pricing: z shape mismatch");
  expect(nm.variance.min().item<double>() > 0.0, "ngarch pricing: variance must be positive");

  const Series nf = pricing_variance_forecast(16, y, np, cfg);
  expect(nf.sizes()[0] == 16, "ngarch pricing: forecast shape mismatch");
  expect(nf.min().item<double>() > 0.0, "ngarch pricing: forecast must be positive");

  const double unc_ng = unconditional_variance(np);
  const double theo_ng = np.omega / (1.0 - np.beta - np.alpha * (1.0 + np.gamma * np.gamma));
  expect(std::abs(unc_ng - theo_ng) < 1e-12, "ngarch pricing: unconditional variance mismatch");
}

void test_garch_pricing_side_and_conversion() {
  const auto cfg = f64_cpu();
  const GarchFit ts_fit{
      .mean = 0.0,
      .omega = 1.0e-6,
      .arch = {0.08},
      .garch = {0.90},
      .variance_floor = 1e-12,
      .stationarity_margin = 1e-6,
      .finite_diff_eps = 1e-5,
  };

  const auto p = to_pricing_params(ts_fit, 0.02, 0.10);
  const Garch11Params gp{
      .omega = p.omega,
      .alpha = p.alpha,
      .beta = p.beta,
      .lambda = p.lambda,
      .r = p.r,
      .variance_floor = p.variance_floor,
      .stationarity_margin = p.stationarity_margin,
  };

  const Series y = ts_fit.simulate(1600, cfg, 20260504ULL);
  const auto gm = pricing_metrics(y, gp, cfg);
  expect(std::isfinite(gm.loglik), "garch pricing: finite loglik expected");
  expect(gm.variance.min().item<double>() > 0.0, "garch pricing: variance must be positive");

  const Series gf = pricing_variance_forecast(12, y, gp, cfg);
  expect(gf.sizes()[0] == 12, "garch pricing: forecast shape mismatch");
  expect(gf.min().item<double>() > 0.0, "garch pricing: forecast must be positive");

  const double unc_g = unconditional_variance(gp);
  const double theo_g = gp.omega / (1.0 - gp.alpha - gp.beta);
  expect(std::abs(unc_g - theo_g) < 1e-12, "garch pricing: unconditional variance mismatch");
}

} // namespace

int main() {
  try {
    test_garch_bridge();
    test_egarch_gjr_bridge();
    test_joint_likelihood();
    test_ngarch_pricing_side();
    test_garch_pricing_side_and_conversion();
    std::cout << "vol_bridge_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "vol_bridge_test failed: " << e.what() << '\n';
    return 1;
  }
}
