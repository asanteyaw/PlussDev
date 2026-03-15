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
    namespace rate = pluss::subsp::math::finance::rates;

    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

    rate::VasicekParams vp;
    vp.kappa = 0.8;
    vp.theta = 0.03;
    vp.sigma = 0.012;
    vp.r0 = 0.025;

    rate::SimOptions ropt;
    ropt.scheme = rate::Scheme::Transition;
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
    ropt.randomized_quasi = true;
    ropt.sobol_scrambling = 3;
    ropt.seed = 20260621ULL;

    const auto v_paths = rate::vasicek_paths(120, 200, 1.0 / 252.0, vp, ropt, cfg, ropt.seed);
    expect(v_paths.rate.isfinite().all().item<double>() == 1.0, "vasicek rates must be finite");

    rate::CirParams cp;
    cp.kappa = 1.1;
    cp.theta = 0.035;
    cp.sigma = 0.18;
    cp.r0 = 0.03;

    ropt.scheme = rate::Scheme::Transition;
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Halton;
    ropt.seed = 20260622ULL;

    const auto c_paths = rate::cir_paths(120, 200, 1.0 / 252.0, cp, ropt, cfg, ropt.seed);
    expect(c_paths.rate.isfinite().all().item<double>() == 1.0, "cir rates must be finite");
    expect(c_paths.rate.min().item<double>() > 0.0, "cir rates must remain positive");

    const double p_v = rate::vasicek_zcb(2.0, vp.r0, vp);
    const double p_c = rate::cir_zcb(2.0, cp.r0, cp);
    expect(std::isfinite(p_v) && p_v > 0.0, "vasicek zcb must be finite and positive");
    expect(std::isfinite(p_c) && p_c > 0.0, "cir zcb must be finite and positive");

    rate::HullWhiteParams hp;
    hp.kappa = 0.7;
    hp.sigma = 0.012;
    hp.r0 = 0.03;
    hp.theta0 = 0.03;
    hp.theta_slope = 0.002;
    ropt.scheme = rate::Scheme::Transition;
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Pseudo;
    ropt.randomized_quasi = false;
    ropt.seed = 20260721ULL;
    const auto h_paths = rate::hull_white_paths(120, 200, 1.0 / 252.0, hp, ropt, cfg, ropt.seed);
    expect(h_paths.rate.isfinite().all().item<double>() == 1.0, "hull-white rates must be finite");
    const double p_h = rate::hull_white_zcb(2.0, hp.r0, hp);
    expect(std::isfinite(p_h) && p_h > 0.0, "hull-white zcb must be finite and positive");

    rate::HjmParams hjm;
    hjm.r0 = 0.03;
    hjm.tenors = {0.25, 0.5, 1.0, 2.0, 3.0};
    hjm.vol = {0.012, 0.011, 0.01, 0.009, 0.008};
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
    ropt.randomized_quasi = true;
    ropt.sobol_scrambling = 3;
    ropt.seed = 20260722ULL;
    const auto hjm_paths = rate::hjm_paths(64, 128, 1.0 / 252.0, hjm, ropt, cfg, ropt.seed);
    expect(hjm_paths.rate.isfinite().all().item<double>() == 1.0, "hjm short-rate path must be finite");
    expect(hjm_paths.forward.isfinite().all().item<double>() == 1.0, "hjm forward path must be finite");
    const double p_hjm = rate::hjm_zcb_mc(1.0, hjm, 3000, 64, ropt, cfg);
    expect(std::isfinite(p_hjm) && p_hjm > 0.0, "hjm zcb mc must be finite and positive");

    ropt.paths_first = true;
    const auto hjm_paths_pf = rate::hjm_paths(64, 128, 1.0 / 252.0, hjm, ropt, cfg, ropt.seed);
    expect(hjm_paths_pf.rate.sizes()[0] == 128 && hjm_paths_pf.rate.sizes()[1] == 65,
           "hjm paths_first rate shape mismatch");
    expect(hjm_paths_pf.forward.sizes()[0] == 128 && hjm_paths_pf.forward.sizes()[1] == 65 &&
               hjm_paths_pf.forward.sizes()[2] == static_cast<std::int64_t>(hjm.tenors.size()),
           "hjm paths_first forward shape mismatch");

    std::cout << "rates_paths_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "rates_paths_test failed: " << e.what() << '\n';
    return 1;
  }
}
