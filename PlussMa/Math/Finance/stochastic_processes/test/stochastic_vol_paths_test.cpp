#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::subsp::math::finance::volatility::HestonParams;
using pluss::subsp::math::finance::volatility::BatesParams;
using pluss::subsp::math::finance::volatility::GbmParams;
using pluss::subsp::math::finance::volatility::RandMethod;
using pluss::subsp::math::finance::volatility::SimOptions;
using pluss::subsp::math::finance::volatility::SimScheme;
using pluss::subsp::math::finance::volatility::SvParams;
using pluss::subsp::math::finance::volatility::bates_paths;
using pluss::subsp::math::finance::volatility::gbm_paths;
using pluss::subsp::math::finance::volatility::heston_paths;
using pluss::subsp::math::finance::volatility::sv_paths;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_heston_paths() {
  const auto cfg = f64_cpu();
  HestonParams p;
  p.kappa = 2.0;
  p.theta = 0.05;
  p.sigma = 0.35;
  p.rho = -0.6;
  p.v0 = 0.04;
  p.mu = 0.01;

  for (const auto scheme : {SimScheme::Euler, SimScheme::Milstein, SimScheme::Milstein2, SimScheme::QuadExp, SimScheme::Transition}) {
    SimOptions opt;
    opt.scheme = scheme;
    opt.method = RandMethod::Pseudo;
    opt.seed = 20260314ULL;
    const auto out = heston_paths(160, 192, 100.0, 1.0 / 252.0, p, opt, cfg, opt.seed);

    expect(out.spot.sizes().size() == 2, "heston spot rank mismatch");
    expect(out.spot.sizes()[0] == 161 && out.spot.sizes()[1] == 192, "heston spot shape mismatch");
    expect(out.variance.sizes()[0] == 161 && out.variance.sizes()[1] == 192, "heston variance shape mismatch");
    expect(out.innovation_spot.sizes()[0] == 160 && out.innovation_spot.sizes()[1] == 192,
           "heston innovation_spot shape mismatch");
    expect(out.innovation_var.sizes()[0] == 160 && out.innovation_var.sizes()[1] == 192,
           "heston innovation_var shape mismatch");
    expect(out.spot.isfinite().all().item<double>() == 1.0, "heston spot has non-finite values");
    expect(out.variance.isfinite().all().item<double>() == 1.0, "heston variance has non-finite values");
    expect(out.variance.min().item<double>() > 0.0, "heston variance must stay positive");
  }

  SimOptions qopt;
  qopt.scheme = SimScheme::QuadExp;
  qopt.method = RandMethod::Sobol;
  qopt.randomized_quasi = true;
  qopt.sobol_scrambling = 3;
  qopt.seed = 20260332ULL;
  qopt.offset = 128;
  const auto qout = heston_paths(80, 96, 100.0, 1.0 / 252.0, p, qopt, cfg, qopt.seed);
  expect(qout.spot.isfinite().all().item<double>() == 1.0, "heston sobol spot has non-finite values");
  expect(qout.variance.min().item<double>() > 0.0, "heston sobol variance must stay positive");

  qopt.paths_first = true;
  const auto qout_pf = heston_paths(80, 96, 100.0, 1.0 / 252.0, p, qopt, cfg, qopt.seed);
  expect(qout_pf.spot.sizes()[0] == 96 && qout_pf.spot.sizes()[1] == 81, "heston paths_first spot shape mismatch");
  expect(qout_pf.innovation_spot.sizes()[0] == 96 && qout_pf.innovation_spot.sizes()[1] == 80,
         "heston paths_first innovation shape mismatch");

  SimOptions vr_opt;
  vr_opt.scheme = SimScheme::Euler;
  vr_opt.method = RandMethod::Pseudo;
  vr_opt.antithetic = true;
  vr_opt.normal_shift = 0.2;
  vr_opt.seed = 20260335ULL;
  const auto vr_out = heston_paths(64, 200, 100.0, 1.0 / 252.0, p, vr_opt, cfg, vr_opt.seed);
  const double z_mean = vr_out.innovation_spot.mean().item<double>();
  expect(std::isfinite(z_mean), "heston antithetic innovation mean must be finite");
  expect(std::abs(z_mean - 0.2) < 0.06, "heston antithetic/shift innovation mean mismatch");
}

void test_sv_paths() {
  const auto cfg = f64_cpu();
  SvParams p;
  p.mu = 0.015;
  p.logvar_mean = -3.1;
  p.phi = 0.97;
  p.vol_of_logvar = 0.22;
  p.rho = -0.45;
  p.logvar0 = -3.3;

  SimOptions opt;
  opt.scheme = SimScheme::Euler;
  opt.method = RandMethod::Halton;
  opt.randomized_quasi = true;
  opt.seed = 20260315ULL;
  opt.offset = 64;
  const auto out = sv_paths(180, 400, 100.0, 1.0 / 252.0, p, opt, cfg, opt.seed);

  expect(out.spot.sizes().size() == 2, "sv spot rank mismatch");
  expect(out.spot.sizes()[0] == 181 && out.spot.sizes()[1] == 400, "sv spot shape mismatch");
  expect(out.variance.sizes()[0] == 181 && out.variance.sizes()[1] == 400, "sv variance shape mismatch");

  expect(out.spot.isfinite().all().item<double>() == 1.0, "sv spot has non-finite values");
  expect(out.variance.isfinite().all().item<double>() == 1.0, "sv variance has non-finite values");
  expect(out.variance.min().item<double>() > 0.0, "sv variance must stay positive");

  opt.paths_first = true;
  const auto out_pf = sv_paths(180, 400, 100.0, 1.0 / 252.0, p, opt, cfg, opt.seed);
  expect(out_pf.spot.sizes()[0] == 400 && out_pf.spot.sizes()[1] == 181, "sv paths_first spot shape mismatch");
  expect(out_pf.innovation_var.sizes()[0] == 400 && out_pf.innovation_var.sizes()[1] == 180,
         "sv paths_first innovation shape mismatch");
}

void test_bates_paths() {
  const auto cfg = f64_cpu();
  BatesParams p;
  p.kappa = 2.0;
  p.theta = 0.05;
  p.sigma = 0.35;
  p.rho = -0.6;
  p.v0 = 0.04;
  p.mu = 0.01;
  p.jump_intensity = 0.2;
  p.jump_mean = -0.08;
  p.jump_vol = 0.2;

  SimOptions opt;
  opt.scheme = SimScheme::QuadExp;
  opt.method = RandMethod::Torus;
  opt.randomized_quasi = true;
  opt.seed = 20260633ULL;
  const auto out = bates_paths(180, 320, 100.0, 1.0 / 252.0, p, opt, cfg, opt.seed);

  expect(out.spot.sizes().size() == 2, "bates spot rank mismatch");
  expect(out.spot.sizes()[0] == 181 && out.spot.sizes()[1] == 320, "bates spot shape mismatch");
  expect(out.variance.sizes()[0] == 181 && out.variance.sizes()[1] == 320, "bates variance shape mismatch");
  expect(out.spot.isfinite().all().item<double>() == 1.0, "bates spot has non-finite values");
  expect(out.variance.isfinite().all().item<double>() == 1.0, "bates variance has non-finite values");
  expect(out.variance.min().item<double>() > 0.0, "bates variance must stay positive");
}

void test_gbm_paths() {
  const auto cfg = f64_cpu();
  GbmParams p;
  p.mu = 0.05;
  p.sigma = 0.2;

  SimOptions opt;
  opt.method = RandMethod::Pseudo;
  opt.antithetic = true;
  opt.seed = 20260731ULL;
  const std::int64_t steps = 252;
  const std::int64_t paths = 50000;
  const double dt = 1.0 / 252.0;
  const double spot0 = 100.0;
  const auto out = gbm_paths(steps, paths, spot0, dt, p, opt, cfg, opt.seed);

  expect(out.spot.sizes().size() == 2, "gbm spot rank mismatch");
  expect(out.spot.sizes()[0] == 253 && out.spot.sizes()[1] == paths, "gbm spot shape mismatch");
  expect(out.variance.sizes()[0] == 253 && out.variance.sizes()[1] == paths, "gbm variance shape mismatch");
  expect(out.innovation_spot.sizes()[0] == steps && out.innovation_spot.sizes()[1] == paths,
         "gbm innovation_spot shape mismatch");
  expect(out.innovation_var.sizes()[0] == steps && out.innovation_var.sizes()[1] == paths,
         "gbm innovation_var shape mismatch");
  expect(out.spot.min().item<double>() > 0.0, "gbm spot must stay positive");
  expect(out.spot.isfinite().all().item<double>() == 1.0, "gbm spot has non-finite values");
  expect(std::abs(out.innovation_var.abs().max().item<double>()) < 1e-15, "gbm innovation_var must be zero");
  expect(std::abs(out.variance.sub(p.sigma * p.sigma).abs().max().item<double>()) < 1e-15,
         "gbm variance must be constant sigma^2");

  const auto idx_last = pluss::make_series(std::vector<std::int64_t>{steps}, {1},
                                           Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  const auto terminal = out.spot.index_select(0, idx_last).reshape({paths});
  const double m_hat = terminal.mean().item<double>();
  const double v_hat = terminal.sub(m_hat).square().mean().item<double>();
  const double t = static_cast<double>(steps) * dt;
  const double m_ref = spot0 * std::exp(p.mu * t);
  const double v_ref = spot0 * spot0 * std::exp(2.0 * p.mu * t) * (std::exp(p.sigma * p.sigma * t) - 1.0);
  const double m_tol = std::max(0.04, 6.0 * std::sqrt(v_ref / static_cast<double>(paths)));
  const double v_tol = std::max(0.8, 8.0 * v_ref * std::sqrt(2.0 / static_cast<double>(paths)));
  expect(std::abs(m_hat - m_ref) < m_tol, "gbm terminal mean mismatch");
  expect(std::abs(v_hat - v_ref) < v_tol, "gbm terminal variance mismatch");

  opt.method = RandMethod::Sobol;
  opt.randomized_quasi = true;
  opt.sobol_scrambling = 3;
  opt.paths_first = true;
  opt.seed = 20260732ULL;
  const auto qout = gbm_paths(64, 256, spot0, dt, p, opt, cfg, opt.seed);
  expect(qout.spot.sizes()[0] == 256 && qout.spot.sizes()[1] == 65, "gbm paths_first spot shape mismatch");
  expect(qout.innovation_spot.sizes()[0] == 256 && qout.innovation_spot.sizes()[1] == 64,
         "gbm paths_first innovation shape mismatch");
}

} // namespace

int main() {
  try {
    test_heston_paths();
    test_sv_paths();
    test_bates_paths();
    test_gbm_paths();
    std::cout << "stochastic_vol_paths_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "stochastic_vol_paths_test failed: " << e.what() << '\n';
    return 1;
  }
}
