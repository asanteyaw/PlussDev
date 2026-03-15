#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

} // namespace

int main() {
  try {
    namespace cal = pluss::subsp::math::finance::calibration;
    namespace prc = pluss::subsp::math::finance::pricing;
    namespace vol = pluss::subsp::math::finance::volatility;

    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

    vol::BatesParams truth;
    truth.kappa = 1.7;
    truth.theta = 0.05;
    truth.sigma = 0.28;
    truth.rho = -0.65;
    truth.v0 = 0.045;
    truth.jump_intensity = 0.15;
    truth.jump_mean = -0.07;
    truth.jump_vol = 0.18;

    prc::MonteCarloOptions mc;
    mc.paths = 800;
    mc.steps = 24;
    mc.scheme = vol::SimScheme::QuadExp;
    mc.rand_method = vol::RandMethod::Sobol;
    mc.randomized_quasi = false;
    mc.sobol_scrambling = 0;
    mc.seed = 20260612ULL;

    std::vector<cal::OptionQuote> quotes;
    for (double k : {90.0, 100.0, 110.0}) {
      cal::OptionQuote q;
      q.type = prc::OptionType::Call;
      q.strike = k;
      q.maturity = 1.0;
      q.market_price = prc::bates_price_mc(q.type, 100.0, k, 0.02, 0.0, q.maturity, truth, mc, cfg);
      q.weight = 1.0;
      quotes.push_back(q);
    }

    vol::BatesParams init = truth;
    init.kappa = 2.4;
    init.theta = 0.09;
    init.sigma = 0.45;
    init.rho = -0.2;
    init.v0 = 0.09;
    init.jump_intensity = 0.45;
    init.jump_mean = 0.05;
    init.jump_vol = 0.35;

    cal::BatesCalibOptions opt;
    opt.random_starts = 1;
    opt.max_iterations = 3;
    opt.seed = 20260613ULL;
    opt.mc = mc;

    const double init_obj = cal::bates_sse(quotes, 100.0, 0.02, 0.0, init, opt, cfg);
    const auto fit = cal::calibrate_bates(quotes, 100.0, 0.02, 0.0, init, opt, cfg);
    expect(std::isfinite(fit.objective), "bates calibration objective must be finite");
    expect(fit.objective <= init_obj + 1e-10, "bates calibration should not worsen objective from initial guess");

    std::cout << "bates_calibration_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "bates_calibration_test failed: " << e.what() << '\n';
    return 1;
  }
}
