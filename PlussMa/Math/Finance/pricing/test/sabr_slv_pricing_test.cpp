#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

} // namespace

int main() {
  try {
    namespace prc = pluss::subsp::math::finance::pricing;
    namespace vol = pluss::subsp::math::finance::volatility;

    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

    vol::SabrParams sabr;
    sabr.alpha0 = 0.2;
    sabr.beta = 0.7;
    sabr.nu = 0.45;
    sabr.rho = -0.3;
    sabr.mu = 0.02;

    prc::MonteCarloOptions mc;
    mc.paths = 6000;
    mc.steps = 96;
    mc.rand_method = vol::RandMethod::Sobol;
    mc.randomized_quasi = true;
    mc.sobol_scrambling = 3;
    mc.seed = 20260711ULL;

    const double sabr_px = prc::detail::sabr_price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 1.0, sabr, mc, cfg);
    expect(std::isfinite(sabr_px) && sabr_px > 0.0, "sabr_price_mc must be finite and positive");

    const prc::SabrApproxOptions aopt;
    const double sabr_black = prc::detail::sabr_black_vol(100.0, 100.0, 1.0, sabr, aopt);
    const double sabr_norm = prc::detail::sabr_normal_vol(100.0, 100.0, 1.0, sabr, aopt);
    expect(std::isfinite(sabr_black) && sabr_black > 0.0, "sabr_black_vol must be finite and positive");
    expect(std::isfinite(sabr_norm) && sabr_norm > 0.0, "sabr_normal_vol must be finite and positive");

    const double sabr_px_approx = prc::detail::sabr_price_approx(prc::OptionType::Call, 100.0, 100.0, 0.02, 1.0, sabr);
    expect(std::isfinite(sabr_px_approx) && sabr_px_approx > 0.0, "sabr_price_approx must be finite and positive");

    vol::SabrParams sabr_bs;
    sabr_bs.alpha0 = 0.25;
    sabr_bs.beta = 1.0;
    sabr_bs.nu = 0.0;
    sabr_bs.rho = 0.0;
    sabr_bs.mu = 0.0;

    prc::MonteCarloOptions mc_bs = mc;
    mc_bs.paths = 30000;
    mc_bs.steps = 128;
    mc_bs.rand_method = vol::RandMethod::Sobol;
    mc_bs.randomized_quasi = true;
    mc_bs.sobol_scrambling = 3;
    mc_bs.seed = 20260712ULL;

    const double px_mc_bs = prc::detail::sabr_price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 1.0, sabr_bs, mc_bs, cfg);
    const double px_bs = prc::detail::black_scholes_price(prc::OptionType::Call, 100.0, 100.0, 0.02, 0.02, 1.0, sabr_bs.alpha0);
    expect(std::isfinite(px_mc_bs) && std::isfinite(px_bs), "sabr/bs comparison must be finite");
    expect(std::abs(px_mc_bs - px_bs) < 0.30, "sabr_price_mc(beta=1,nu=0) must agree with Black-Scholes");

    vol::SimOptions sim_opt;
    sim_opt.method = vol::RandMethod::Sobol;
    sim_opt.randomized_quasi = true;
    sim_opt.sobol_scrambling = 3;
    sim_opt.seed = 20260713ULL;
    const auto sabr_paths = vol::detail::sabr_paths(96, 20000, 100.0, 1.0 / 96.0, sabr, sim_opt, cfg, sim_opt.seed);
    const double mean_alpha_t = sabr_paths.variance.select(0, 96).sqrt().mean().item<double>();
    expect(std::isfinite(mean_alpha_t), "sabr alpha path mean must be finite");
    expect(std::abs(mean_alpha_t - sabr.alpha0) < 0.02, "sabr alpha process should preserve E[alpha_t] approximately");

    vol::SlvParams slv;
    slv.mu = 0.0;
    slv.kappa = 1.5;
    slv.theta = 0.04;
    slv.sigma = 0.2;
    slv.rho = -0.6;
    slv.v0 = 0.04;
    slv.local_vol_form = vol::LocalVolForm::Cev;
    slv.local_vol_level = 0.9;
    slv.local_vol_curvature = 0.2;

    const double slv_px = prc::detail::slv_price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, slv, mc, cfg);
    expect(std::isfinite(slv_px) && slv_px > 0.0, "slv_price_mc must be finite and positive");

    std::cout << "sabr_slv_pricing_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "sabr_slv_pricing_test failed: " << e.what() << '\n';
    return 1;
  }
}
