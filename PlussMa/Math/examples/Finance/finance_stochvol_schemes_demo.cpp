#include <pluss/ma.h>

#include <cmath>
#include <iostream>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  pluss::subsp::math::finance::volatility::HestonParams hp;
  hp.kappa = 1.9;
  hp.theta = 0.05;
  hp.sigma = 0.30;
  hp.rho = -0.6;
  hp.v0 = 0.04;
  hp.mu = 0.01;

  pluss::subsp::math::finance::volatility::SvParams sp;
  sp.mu = 0.01;
  sp.logvar_mean = -3.2;
  sp.phi = 0.97;
  sp.vol_of_logvar = 0.24;
  sp.rho = -0.55;
  sp.logvar0 = -3.1;

  pluss::subsp::math::finance::volatility::SimOptions opt;
  opt.seed = 20260401ULL;

  for (const auto scheme : {
           pluss::subsp::math::finance::volatility::SimScheme::Euler,
           pluss::subsp::math::finance::volatility::SimScheme::Milstein,
           pluss::subsp::math::finance::volatility::SimScheme::Milstein2,
           pluss::subsp::math::finance::volatility::SimScheme::QuadExp,
           pluss::subsp::math::finance::volatility::SimScheme::Transition}) {
    opt.scheme = scheme;
    opt.method = pluss::subsp::math::finance::volatility::RandMethod::Pseudo;
    const auto p = pluss::subsp::math::finance::volatility::heston_paths(64, 4, 100.0, 1.0 / 252.0, hp, opt, cfg, opt.seed);
    std::cout << "heston scheme=" << static_cast<int>(scheme) << "\n";
    std::cout << "spot=" << p.spot << "\n";
    std::cout << "variance=" << p.variance << "\n";
  }

  opt.scheme = pluss::subsp::math::finance::volatility::SimScheme::QuadExp;
  opt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  opt.randomized_quasi = true;
  opt.sobol_scrambling = 3;
  opt.offset = 128;
  const auto p_sob = pluss::subsp::math::finance::volatility::heston_paths(64, 4, 100.0, 1.0 / 252.0, hp, opt, cfg, opt.seed);
  std::cout << "heston sobol randomized\n";
  std::cout << "spot=" << p_sob.spot << "\n";

  opt.method = pluss::subsp::math::finance::volatility::RandMethod::Halton;
  const auto p_hal = pluss::subsp::math::finance::volatility::heston_paths(64, 4, 100.0, 1.0 / 252.0, hp, opt, cfg, opt.seed);
  std::cout << "heston halton randomized\n";
  std::cout << "spot=" << p_hal.spot << "\n";

  opt.method = pluss::subsp::math::finance::volatility::RandMethod::Torus;
  const auto p_tor = pluss::subsp::math::finance::volatility::heston_paths(64, 4, 100.0, 1.0 / 252.0, hp, opt, cfg, opt.seed);
  std::cout << "heston torus randomized\n";
  std::cout << "spot=" << p_tor.spot << "\n";

  opt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  const auto p_sv = pluss::subsp::math::finance::volatility::sv_paths(64, 4, 100.0, 1.0 / 252.0, sp, opt, cfg, opt.seed);
  std::cout << "sv sobol randomized\n";
  std::cout << "spot=" << p_sv.spot << "\n";
  std::cout << "variance=" << p_sv.variance << "\n";

  pluss::subsp::math::finance::volatility::SabrParams qsp;
  qsp.alpha0 = 0.01;
  qsp.beta = 0.3;
  qsp.nu = 0.03;
  qsp.rho = -0.2;
  qsp.mu = 0.0;
  opt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  const auto p_sabr = pluss::subsp::math::finance::volatility::sabr_paths(64, 4, 100.0, 1.0 / 252.0, qsp, opt, cfg, opt.seed);
  std::cout << "sabr sobol randomized\n";
  std::cout << "forward=" << p_sabr.spot << "\n";
  std::cout << "alpha2=" << p_sabr.variance << "\n";

  pluss::subsp::math::finance::volatility::SlvParams slvp;
  slvp.mu = 0.01;
  slvp.kappa = 1.6;
  slvp.theta = 0.04;
  slvp.sigma = 0.25;
  slvp.rho = -0.55;
  slvp.v0 = 0.04;
  slvp.local_vol_form = pluss::subsp::math::finance::volatility::LocalVolForm::Cev;
  slvp.local_vol_level = 0.95;
  slvp.local_vol_curvature = 0.2;
  opt.method = pluss::subsp::math::finance::volatility::RandMethod::Halton;
  const auto p_slv = pluss::subsp::math::finance::volatility::slv_paths(64, 4, 100.0, 1.0 / 252.0, slvp, opt, cfg, opt.seed);
  std::cout << "slv halton randomized\n";
  std::cout << "spot=" << p_slv.spot << "\n";
  std::cout << "variance=" << p_slv.variance << "\n";

  pluss::subsp::math::finance::pricing::MonteCarloOptions mc;
  mc.paths = 12000;
  mc.steps = 128;
  mc.scheme = pluss::subsp::math::finance::volatility::SimScheme::QuadExp;
  mc.rand_method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.offset = 256;
  mc.seed = 20260402ULL;

  const double h_mc = pluss::subsp::math::finance::pricing::heston_price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      hp,
      mc,
      cfg);
  const double sv_mc = pluss::subsp::math::finance::pricing::sv_price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      sp,
      mc,
      cfg);

  mc.antithetic = true;
  const double h_mc_anti = pluss::subsp::math::finance::pricing::heston_price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      hp,
      mc,
      cfg);

  mc.control_variate = true;
  mc.control_vol = std::sqrt(hp.theta);
  const double h_mc_cv = pluss::subsp::math::finance::pricing::heston_price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      hp,
      mc,
      cfg);

  mc.importance_sampling = true;
  mc.importance_shift = 0.15;
  const double h_mc_is = pluss::subsp::math::finance::pricing::heston_price_mc(
      pluss::subsp::math::finance::pricing::OptionType::Call,
      100.0,
      100.0,
      0.02,
      0.0,
      1.0,
      hp,
      mc,
      cfg);

  std::cout << "heston_price_mc(sobol,qe)=" << h_mc << "\n";
  std::cout << "sv_price_mc(sobol)=" << sv_mc << "\n";
  std::cout << "heston_price_mc(antithetic)=" << h_mc_anti << "\n";
  std::cout << "heston_price_mc(control_variate)=" << h_mc_cv << "\n";
  std::cout << "heston_price_mc(importance_sampling)=" << h_mc_is << "\n";
  return 0;
}
