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

    const double sabr_px = prc::sabr_price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 1.0, sabr, mc, cfg);
    expect(std::isfinite(sabr_px) && sabr_px > 0.0, "sabr_price_mc must be finite and positive");

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

    const double slv_px = prc::slv_price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, slv, mc, cfg);
    expect(std::isfinite(slv_px) && slv_px > 0.0, "slv_price_mc must be finite and positive");

    std::cout << "sabr_slv_pricing_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "sabr_slv_pricing_test failed: " << e.what() << '\n';
    return 1;
  }
}
