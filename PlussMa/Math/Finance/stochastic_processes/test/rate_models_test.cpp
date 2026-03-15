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
    namespace mdl = pluss::subsp::math::finance::models;
    namespace rate = pluss::subsp::math::finance::rates;

    const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

    rate::VasicekParams vp;
    vp.kappa = 0.7;
    vp.theta = 0.03;
    vp.sigma = 0.01;
    vp.r0 = 0.028;

    mdl::VasicekRate vmod{vp};
    rate::SimOptions ropt;
    ropt.scheme = rate::Scheme::Transition;
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Pseudo;
    ropt.seed = 20260623ULL;
    const auto vpaths = vmod.paths(100, 128, 1.0 / 252.0, ropt, cfg, ropt.seed);
    expect(vpaths.rate.isfinite().all().item<double>() == 1.0, "vasicek model paths must be finite");
    expect(std::isfinite(vmod.zcb(1.0, vp.r0)) && vmod.zcb(1.0, vp.r0) > 0.0, "vasicek zcb invalid");

    std::vector<cal::RateQuote> curve_v;
    for (double t : {0.5, 1.0, 2.0, 3.0}) {
      cal::RateQuote q;
      q.maturity = t;
      q.zcb_price = rate::vasicek_zcb(t, vp.r0, vp);
      q.weight = 1.0;
      curve_v.push_back(q);
    }
    cal::VasicekCalibOptions vopt;
    vopt.random_starts = 2;
    vopt.max_iterations = 30;
    vopt.seed = 20260624ULL;
    const auto vfit = vmod.calibrate_inplace(curve_v, vopt);
    expect(std::isfinite(vfit.objective), "vasicek calibration objective invalid");

    rate::CirParams cp;
    cp.kappa = 1.1;
    cp.theta = 0.035;
    cp.sigma = 0.2;
    cp.r0 = 0.03;
    cp.rate_floor = 1e-12;

    mdl::CirRate cmod{cp};
    ropt.scheme = rate::Scheme::Transition;
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Torus;
    ropt.randomized_quasi = true;
    ropt.seed = 20260625ULL;
    const auto cpaths = cmod.paths(100, 128, 1.0 / 252.0, ropt, cfg, ropt.seed);
    expect(cpaths.rate.isfinite().all().item<double>() == 1.0, "cir model paths must be finite");
    expect(cpaths.rate.min().item<double>() > 0.0, "cir model rates must stay positive");
    expect(std::isfinite(cmod.zcb(1.0, cp.r0)) && cmod.zcb(1.0, cp.r0) > 0.0, "cir zcb invalid");

    std::vector<cal::RateQuote> curve_c;
    for (double t : {0.5, 1.0, 2.0, 3.0}) {
      cal::RateQuote q;
      q.maturity = t;
      q.zcb_price = rate::cir_zcb(t, cp.r0, cp);
      q.weight = 1.0;
      curve_c.push_back(q);
    }
    cal::CirCalibOptions copt;
    copt.random_starts = 2;
    copt.max_iterations = 30;
    copt.seed = 20260626ULL;
    const auto cfit = cmod.calibrate_inplace(curve_c, copt);
    expect(std::isfinite(cfit.objective), "cir calibration objective invalid");

    rate::HullWhiteParams hp;
    hp.kappa = 0.7;
    hp.sigma = 0.012;
    hp.r0 = 0.03;
    hp.theta0 = 0.03;
    hp.theta_slope = 0.001;
    mdl::HullWhiteRate hmod{hp};
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Pseudo;
    ropt.randomized_quasi = false;
    ropt.seed = 20260731ULL;
    const auto hpaths = hmod.paths(100, 128, 1.0 / 252.0, ropt, cfg, ropt.seed);
    expect(hpaths.rate.isfinite().all().item<double>() == 1.0, "hull-white model paths must be finite");
    expect(std::isfinite(hmod.zcb(1.0, hp.r0)) && hmod.zcb(1.0, hp.r0) > 0.0, "hull-white zcb invalid");

    std::vector<cal::RateQuote> curve_h;
    for (double t : {0.5, 1.0, 2.0, 3.0}) {
      cal::RateQuote q;
      q.maturity = t;
      q.zcb_price = rate::hull_white_zcb(t, hp.r0, hp);
      q.weight = 1.0;
      curve_h.push_back(q);
    }
    cal::HullWhiteCalibOptions hopt;
    hopt.random_starts = 2;
    hopt.max_iterations = 25;
    hopt.seed = 20260732ULL;
    const auto hfit = hmod.calibrate_inplace(curve_h, hopt);
    expect(std::isfinite(hfit.objective), "hull-white calibration objective invalid");

    rate::HjmParams hjm;
    hjm.r0 = 0.03;
    hjm.tenors = {0.25, 0.5, 1.0, 2.0, 3.0};
    hjm.vol = {0.012, 0.011, 0.01, 0.009, 0.008};
    mdl::HjmRate hjm_mod{hjm};
    ropt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
    ropt.randomized_quasi = true;
    ropt.sobol_scrambling = 3;
    ropt.seed = 20260733ULL;
    const auto hjm_paths = hjm_mod.paths(64, 64, 1.0 / 252.0, ropt, cfg, ropt.seed);
    expect(hjm_paths.rate.isfinite().all().item<double>() == 1.0, "hjm model short rate must be finite");
    expect(hjm_paths.forward.isfinite().all().item<double>() == 1.0, "hjm model forward must be finite");
    const double hjm_zcb = hjm_mod.zcb_mc(1.0, 3000, 64, ropt, cfg);
    expect(std::isfinite(hjm_zcb) && hjm_zcb > 0.0, "hjm model zcb_mc invalid");

    std::vector<cal::RateQuote> curve_j;
    for (double t : {0.5, 1.0, 2.0}) {
      cal::RateQuote q;
      q.maturity = t;
      q.zcb_price = rate::hjm_zcb_mc(t, hjm, 3000, 64, ropt, cfg);
      q.weight = 1.0;
      curve_j.push_back(q);
    }
    cal::HjmCalibOptions jopt;
    jopt.random_starts = 1;
    jopt.max_iterations = 5;
    jopt.seed = 20260734ULL;
    const auto jfit = hjm_mod.calibrate_inplace(curve_j, jopt);
    expect(std::isfinite(jfit.objective), "hjm calibration objective invalid");

    std::cout << "rate_models_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "rate_models_test failed: " << e.what() << '\n';
    return 1;
  }
}
