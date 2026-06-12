#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::subsp::math::finance::models::Bates;
using pluss::subsp::math::finance::models::Heston;
using pluss::subsp::math::finance::models::HestonNandi;
using pluss::subsp::math::finance::models::RoughBergomi;
using pluss::subsp::math::finance::models::RoughHeston;
using pluss::subsp::math::finance::models::StochVolBase;
using pluss::subsp::math::finance::models::Sv;
using pluss::subsp::math::finance::volatility::BatesParams;
using pluss::subsp::math::finance::volatility::HestonNandiParams;
using pluss::subsp::math::finance::volatility::RoughBergomiParams;
using pluss::subsp::math::finance::volatility::RoughHestonParams;
using pluss::subsp::math::finance::pricing::HestonIntegrationOptions;
using pluss::subsp::math::finance::pricing::MonteCarloOptions;
using pluss::subsp::math::finance::pricing::OptionType;
using pluss::subsp::math::finance::volatility::HestonParams;
using pluss::subsp::math::finance::volatility::RandMethod;
using pluss::subsp::math::finance::volatility::SimOptions;
using pluss::subsp::math::finance::volatility::SimScheme;
using pluss::subsp::math::finance::volatility::SvParams;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_heston_object_lane() {
  const auto cfg = f64_cpu();

  HestonParams hp;
  hp.kappa = 1.8;
  hp.theta = 0.05;
  hp.sigma = 0.28;
  hp.rho = -0.65;
  hp.v0 = 0.045;

  Heston model{hp};

  HestonIntegrationOptions int_opt;
  int_opt.integration_steps = 240;
  int_opt.integration_u_max = 100.0;

  const double p_cf = model.price(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, int_opt);
  expect(std::isfinite(p_cf) && p_cf > 0.0, "heston object price should be finite and positive");

  MonteCarloOptions mc;
  mc.paths = 3000;
  mc.steps = 64;
  mc.scheme = SimScheme::QuadExp;
  mc.rand_method = RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.seed = 20260411ULL;

  const double p_mc = model.price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);
  expect(std::isfinite(p_mc) && p_mc > 0.0, "heston object price_mc should be finite and positive");

  SimOptions sim_opt;
  sim_opt.scheme = SimScheme::Milstein;
  sim_opt.method = RandMethod::Pseudo;
  sim_opt.seed = 20260412ULL;
  const auto paths = model.paths(80, 128, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);
  expect(paths.spot.isfinite().all().item<double>() == 1.0, "heston object paths spot must be finite");
  expect(paths.variance.min().item<double>() > 0.0, "heston object paths variance must be positive");

  const StochVolBase& base = model;
  const double p_mc_base = base.price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);
  expect(std::isfinite(p_mc_base) && p_mc_base > 0.0, "stochvol base price_mc should be finite and positive");

}

void test_sv_object_lane() {
  const auto cfg = f64_cpu();

  SvParams sp;
  sp.mu = 0.0;
  sp.logvar_mean = -3.1;
  sp.phi = 0.97;
  sp.vol_of_logvar = 0.24;
  sp.rho = -0.5;
  sp.logvar0 = -3.2;

  Sv model{sp};

  MonteCarloOptions mc;
  mc.paths = 2500;
  mc.steps = 64;
  mc.rand_method = RandMethod::Halton;
  mc.randomized_quasi = true;
  mc.seed = 20260414ULL;

  const double p_mc = model.price(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);
  expect(std::isfinite(p_mc) && p_mc > 0.0, "sv object price should be finite and positive");

  SimOptions sim_opt;
  sim_opt.method = RandMethod::Torus;
  sim_opt.randomized_quasi = true;
  sim_opt.seed = 20260415ULL;
  const auto paths = model.paths(80, 96, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);
  expect(paths.spot.isfinite().all().item<double>() == 1.0, "sv object paths spot must be finite");
  expect(paths.variance.min().item<double>() > 0.0, "sv object paths variance must be positive");
}

void test_bates_object_lane() {
  const auto cfg = f64_cpu();

  BatesParams bp;
  bp.kappa = 1.8;
  bp.theta = 0.05;
  bp.sigma = 0.28;
  bp.rho = -0.6;
  bp.v0 = 0.045;
  bp.jump_intensity = 0.2;
  bp.jump_mean = -0.08;
  bp.jump_vol = 0.2;

  Bates model{bp};

  MonteCarloOptions mc;
  mc.paths = 4000;
  mc.steps = 80;
  mc.scheme = SimScheme::QuadExp;
  mc.rand_method = RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.seed = 20260631ULL;

  const double p_mc = model.price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);
  expect(std::isfinite(p_mc) && p_mc > 0.0, "bates object price_mc should be finite and positive");

  SimOptions sim_opt;
  sim_opt.scheme = SimScheme::Milstein;
  sim_opt.method = RandMethod::Pseudo;
  sim_opt.seed = 20260632ULL;
  const auto paths = model.paths(80, 96, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);
  expect(paths.spot.isfinite().all().item<double>() == 1.0, "bates object paths spot must be finite");
  expect(paths.variance.min().item<double>() > 0.0, "bates object paths variance must be positive");
}

void test_heston_nandi_object_lane() {
  const auto cfg = f64_cpu();

  HestonNandiParams hp;
  hp.omega = 1e-6;
  hp.alpha = 2.0e-5;
  hp.beta = 0.92;
  hp.gamma = 5.0;
  hp.lambda = 0.0;
  hp.r = 0.0;
  hp.initial_variance = 0.00015;

  HestonNandi model{hp};

  MonteCarloOptions mc;
  mc.paths = 5000;
  mc.steps = 252;
  mc.rand_method = RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.seed = 20260633ULL;

  const double p_mc = model.price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);
  expect(std::isfinite(p_mc) && p_mc > 0.0, "heston_nandi object price_mc should be finite and positive");

  SimOptions sim_opt;
  sim_opt.method = RandMethod::Pseudo;
  sim_opt.seed = 20260634ULL;
  const auto paths = model.paths(80, 96, 100.0, 1.0 / 252.0, sim_opt, cfg, sim_opt.seed);
  expect(paths.spot.isfinite().all().item<double>() == 1.0, "heston_nandi object paths spot must be finite");
  expect(paths.variance.min().item<double>() > 0.0, "heston_nandi object paths variance must be positive");
}

void test_rough_models_lane() {
  const auto cfg = f64_cpu();

  RoughHestonParams rhp;
  rhp.kappa = 1.5;
  rhp.theta = 0.04;
  rhp.sigma = 0.3;
  rhp.alpha = -0.2;
  rhp.rho = -0.7;
  rhp.v0 = 0.04;
  RoughHeston rh{rhp};

  RoughBergomiParams rbp;
  rbp.xi = 0.04;
  rbp.eta = 1.6;
  rbp.alpha = -0.4;
  rbp.rho = -0.7;
  RoughBergomi rb{rbp};

  MonteCarloOptions mc;
  mc.paths = 4000;
  mc.steps = 128;
  mc.rand_method = RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.seed = 20260731ULL;

  const double rh_px = rh.price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);
  const double rb_px = rb.price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, mc, cfg);
  expect(std::isfinite(rh_px) && rh_px > 0.0, "rough_heston object price_mc should be finite and positive");
  expect(std::isfinite(rb_px) && rb_px > 0.0, "rough_bergomi object price_mc should be finite and positive");
}

} // namespace

int main() {
  try {
    test_heston_object_lane();
    test_sv_object_lane();
    test_bates_object_lane();
    test_heston_nandi_object_lane();
    test_rough_models_lane();
    std::cout << "stochvol_models_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "stochvol_models_test failed: " << e.what() << '\n';
    return 1;
  }
}
