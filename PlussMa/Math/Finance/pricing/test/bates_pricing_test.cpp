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

    vol::BatesParams p;
    p.kappa = 1.8;
    p.theta = 0.05;
    p.sigma = 0.3;
    p.rho = -0.6;
    p.v0 = 0.04;
    p.jump_intensity = 0.2;
    p.jump_mean = -0.08;
    p.jump_vol = 0.2;

    prc::MonteCarloOptions mc;
    mc.paths = 12000;
    mc.steps = 120;
    mc.scheme = vol::SimScheme::QuadExp;
    mc.rand_method = vol::RandMethod::Sobol;
    mc.randomized_quasi = true;
    mc.sobol_scrambling = 3;
    mc.seed = 20260611ULL;

    const double px = prc::bates_price_mc(prc::OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, p, mc, cfg);
    expect(std::isfinite(px) && px > 0.0, "bates_price_mc should be finite and positive");

    std::cout << "bates_pricing_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "bates_pricing_test failed: " << e.what() << '\n';
    return 1;
  }
}
