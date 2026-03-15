#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::subsp::math::finance::pricing::HestonCfMode;
using pluss::subsp::math::finance::pricing::HestonIntegrationOptions;
using pluss::subsp::math::finance::pricing::HestonNandiIntegrationOptions;
using pluss::subsp::math::finance::pricing::MonteCarloOptions;
using pluss::subsp::math::finance::pricing::OptionType;
using pluss::subsp::math::finance::pricing::black_scholes_price;
using pluss::subsp::math::finance::pricing::heston_nandi_price;
using pluss::subsp::math::finance::pricing::heston_nandi_price_mc;
using pluss::subsp::math::finance::pricing::heston_price;
using pluss::subsp::math::finance::pricing::heston_price_mc;
using pluss::subsp::math::finance::pricing::sv_price_mc;
using pluss::subsp::math::finance::volatility::HestonParams;
using pluss::subsp::math::finance::volatility::HestonNandiParams;
using pluss::subsp::math::finance::volatility::RandMethod;
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

void test_heston_cf_put_call_parity() {
  HestonParams p;
  p.kappa = 1.9;
  p.theta = 0.04;
  p.sigma = 0.33;
  p.rho = -0.7;
  p.v0 = 0.05;

  HestonIntegrationOptions int_opt;
  int_opt.integration_steps = 1200;
  int_opt.integration_u_max = 130.0;
  int_opt.mode = HestonCfMode::LittleTrap;

  const double s = 100.0;
  const double k = 105.0;
  const double r = 0.02;
  const double q = 0.01;
  const double t = 1.5;

  const double call = heston_price(OptionType::Call, s, k, r, q, t, p, int_opt);
  const double put = heston_price(OptionType::Put, s, k, r, q, t, p, int_opt);

  const double lhs = call - put;
  const double rhs = s * std::exp(-q * t) - k * std::exp(-r * t);

  expect(std::isfinite(call) && call >= 0.0, "heston call should be finite and non-negative");
  expect(std::isfinite(put) && put >= 0.0, "heston put should be finite and non-negative");
  expect(std::abs(lhs - rhs) < 2e-4, "heston put-call parity mismatch");
}

void test_heston_cf_near_bs_in_low_volofvol_limit() {
  HestonParams p;
  p.kappa = 3.0;
  p.theta = 0.04;
  p.sigma = 1e-3;
  p.rho = -0.2;
  p.v0 = 0.04;

  HestonIntegrationOptions int_opt;
  int_opt.integration_steps = 1000;
  int_opt.integration_u_max = 120.0;

  const double s = 100.0;
  const double k = 95.0;
  const double r = 0.03;
  const double q = 0.01;
  const double t = 1.0;

  const double heston_call = heston_price(OptionType::Call, s, k, r, q, t, p, int_opt);
  const double bs_call = black_scholes_price(OptionType::Call, s, k, r, q, t, std::sqrt(p.theta));

  expect(std::isfinite(heston_call), "heston cf low-volofvol price should be finite");
  expect(std::abs(heston_call - bs_call) < 0.5, "heston low-volofvol should be close to Black-Scholes");
}

void test_heston_and_sv_mc_prices() {
  const auto cfg = f64_cpu();

  HestonParams hp;
  hp.kappa = 2.0;
  hp.theta = 0.045;
  hp.sigma = 0.30;
  hp.rho = -0.6;
  hp.v0 = 0.04;

  SvParams sp;
  sp.mu = 0.0;
  sp.logvar_mean = -3.0;
  sp.phi = 0.98;
  sp.vol_of_logvar = 0.22;
  sp.rho = -0.5;
  sp.logvar0 = -3.2;

  MonteCarloOptions mc;
  mc.paths = 10000;
  mc.steps = 120;
  mc.seed = 20260316ULL;
  mc.scheme = SimScheme::QuadExp;
  mc.rand_method = RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.offset = 256;

  const double hc = heston_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.00, 1.0, hp, mc, cfg);
  const double hp_put = heston_price_mc(OptionType::Put, 100.0, 100.0, 0.02, 0.00, 1.0, hp, mc, cfg);
  const double sc = sv_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.00, 1.0, sp, mc, cfg);

  expect(std::isfinite(hc) && hc > 0.0, "heston MC call should be finite and positive");
  expect(std::isfinite(hp_put) && hp_put > 0.0, "heston MC put should be finite and positive");
  expect(std::isfinite(sc) && sc > 0.0, "sv MC call should be finite and positive");

  mc.scheme = SimScheme::Milstein;
  mc.rand_method = RandMethod::Pseudo;
  mc.randomized_quasi = false;
  const double hc_milstein = heston_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.00, 1.0, hp, mc, cfg);
  expect(std::isfinite(hc_milstein) && hc_milstein > 0.0, "heston Milstein MC call should be finite and positive");

  mc.scheme = SimScheme::Euler;
  mc.paths = 16000;
  mc.steps = 128;
  mc.seed = 20260317ULL;
  mc.antithetic = true;
  const double hc_anti = heston_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.00, 1.0, hp, mc, cfg);
  expect(std::isfinite(hc_anti) && hc_anti > 0.0, "heston antithetic MC call should be finite and positive");

  mc.control_variate = true;
  mc.control_vol = std::sqrt(hp.theta);
  const double hc_cv = heston_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.00, 1.0, hp, mc, cfg);
  expect(std::isfinite(hc_cv) && hc_cv > 0.0, "heston control-variate MC call should be finite and positive");
  expect(std::abs(hc_cv - hc_anti) < 3.0, "heston control-variate estimate should remain in stable range");

  mc.importance_sampling = true;
  mc.importance_shift = 0.15;
  const double hc_is = heston_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.00, 1.0, hp, mc, cfg);
  expect(std::isfinite(hc_is) && hc_is > 0.0, "heston importance-sampling MC call should be finite and positive");

  mc.control_variate = false;
  const double sc_is = sv_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.00, 1.0, sp, mc, cfg);
  expect(std::isfinite(sc_is) && sc_is > 0.0, "sv importance-sampling MC call should be finite and positive");
}

void test_heston_nandi_mc_price() {
  const auto cfg = f64_cpu();
  HestonNandiParams p;
  p.omega = 1e-6;
  p.alpha = 2.0e-5;
  p.beta = 0.92;
  p.gamma = 5.0;
  p.lambda = 0.0;
  p.r = 0.0;
  p.initial_variance = 0.00015;

  HestonNandiIntegrationOptions cf_opt;
  cf_opt.integration_steps = 1600;
  cf_opt.integration_u_max = 180.0;
  cf_opt.periods_per_year = 252;
  const double call_cf = heston_nandi_price(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, p, cf_opt);
  const double put_cf = heston_nandi_price(OptionType::Put, 100.0, 100.0, 0.02, 0.0, 1.0, p, cf_opt);
  const double parity = 100.0 * std::exp(-0.0) - 100.0 * std::exp(-0.02);
  expect(std::isfinite(call_cf) && call_cf > 0.0, "heston_nandi_price call should be finite and positive");
  expect(std::isfinite(put_cf) && put_cf > 0.0, "heston_nandi_price put should be finite and positive");
  expect(std::abs((call_cf - put_cf) - parity) < 2e-2, "heston_nandi_price put-call parity mismatch");

  MonteCarloOptions mc;
  mc.paths = 20000;
  mc.steps = 252;
  mc.rand_method = RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.control_variate = true;
  mc.seed = 20260321ULL;

  const double call = heston_nandi_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, p, mc, cfg);
  const double put = heston_nandi_price_mc(OptionType::Put, 100.0, 100.0, 0.02, 0.0, 1.0, p, mc, cfg);
  expect(std::isfinite(call) && call > 0.0, "heston_nandi_price_mc call should be finite and positive");
  expect(std::isfinite(put) && put > 0.0, "heston_nandi_price_mc put should be finite and positive");
  expect(std::abs(call - call_cf) < 1.5, "heston_nandi_price_mc and closed-form call should be reasonably aligned");
}

} // namespace

int main() {
  try {
    test_heston_cf_put_call_parity();
    test_heston_cf_near_bs_in_low_volofvol_limit();
    test_heston_and_sv_mc_prices();
    test_heston_nandi_mc_price();
    std::cout << "heston_sv_pricing_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "heston_sv_pricing_test failed: " << e.what() << '\n';
    return 1;
  }
}
