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


// The discount factor E[exp(-int r)] of every path generator must match its closed form. Hull-White has a level theta0 + theta_slope t, so the
// Euler and the transition scheme must agree with each other and with the formula, also when the level moves.
void test_zero_coupon_prices_match_simulation() {
  namespace rate = pluss::subsp::math::finance::rates;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t steps = 120, paths = 80000;
  const double maturity = 3.0, dt = maturity / static_cast<double>(steps);
  const auto discount = [&](const pluss::Series& rate_paths) {
    const auto integral = rate_paths.index_select(0, pluss::Series::arange(0, steps, 1, pluss::ScalarType::Int64)).sum(0, false) * dt;
    return (integral * -1.0).exp().mean().item<double>();
  };
  for (const double slope : {0.0, 0.01}) {
    rate::HullWhiteParams hp;
    hp.theta_slope = slope;
    for (const auto scheme : {rate::Scheme::Euler, rate::Scheme::Transition}) {
      rate::SimOptions opt;
      opt.scheme = scheme;
      opt.seed = 20261030ULL;
      const double simulated = discount(rate::hull_white_paths(steps, paths, dt, hp, opt, cfg, opt.seed).rate);
      expect(std::abs(simulated - rate::hull_white_zcb(maturity, hp.r0, hp)) < 6e-4,
             "hull-white simulated discount factor must match hull_white_zcb");
    }
  }
}

// With a level that moves linearly the transition scheme is exact whatever the step: E r_t = theta0 + slope (t - 1/k) + (r0 - theta0 + slope/k) e^{-k t}.
// A level taken at the midpoint of each step is wrong by order slope * k * dt^2 per step, visible with a step of a year.
void test_hull_white_transition_is_exact_for_a_sloping_level() {
  namespace rate = pluss::subsp::math::finance::rates;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  rate::HullWhiteParams hp;
  hp.kappa = 2.0;
  hp.sigma = 0.005;
  hp.r0 = 0.03;
  hp.theta0 = 0.03;
  hp.theta_slope = 0.01;
  rate::SimOptions opt;
  opt.scheme = rate::Scheme::Transition;
  opt.seed = 5;
  const auto sim = rate::hull_white_paths(5, 200000, 1.0, hp, opt, cfg, opt.seed).rate;
  for (const int t : {1, 3, 5}) {
    const double exact = hp.theta0 + hp.theta_slope * (t - 1.0 / hp.kappa) + (hp.r0 - hp.theta0 + hp.theta_slope / hp.kappa) * std::exp(-hp.kappa * t);
    const double mean = sim.select(0, t).mean().item<double>();
    expect(std::abs(mean - exact) < 5e-5, "the Hull-White transition mean must be exact for a linear level");
  }
}

// Quasi-random sequences must have one point per path: with the points taken as the time steps the discount factor error was ten times that of
// pseudo-random numbers. The unscrambled sequence must now beat the pseudo-random error (about 2e-4 here) and the randomised one must be accurate.
void test_quasi_random_rate_paths_are_accurate() {
  namespace rate = pluss::subsp::math::finance::rates;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const rate::VasicekParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  const std::int64_t steps = 12, paths = 4096;
  const double maturity = 3.0, dt = maturity / static_cast<double>(steps), exact = rate::vasicek_zcb(maturity, p.r0, p);
  for (const bool randomised : {false, true}) {
    rate::SimOptions opt;
    opt.scheme = rate::Scheme::Transition;
    opt.method = pluss::subsp::math::finance::volatility::RandMethod::Sobol;
    opt.randomized_quasi = randomised;
    opt.sobol_scrambling = randomised ? 3 : 0;
    opt.seed = 11ULL;
    const auto rate_paths = rate::vasicek_paths(steps, paths, dt, p, opt, cfg, opt.seed).rate;
    const auto integral = (rate_paths.sum(0, false) - (rate_paths.select(0, 0) + rate_paths.select(0, steps)) * 0.5) * dt;  // trapezoid rule
    const double estimate = (integral * -1.0).exp().mean().item<double>();
    expect(std::abs(estimate - exact) < (randomised ? 1e-3 : 1e-4), "sobol rate paths must price the discount factor accurately");
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

    // No arbitrage: started from a flat curve, the discounted bond price must be exp(-r0 T) up to Monte Carlo and grid error. The drift
    // sigma(tau) int_0^tau sigma and the transport along the tenor axis are both needed for this; with the tail-sum drift and no transport
    // the one-year-to-three-year yields came out at 3.01%..3.06%.
    {
      rate::SimOptions plain;
      plain.seed = 3;
      for (double maturity : {1.0, 2.0}) {
        const double price = rate::hjm_zcb_mc(maturity, hjm, 100000, 252, plain, cfg);
        expect(std::abs(price - std::exp(-hjm.r0 * maturity)) < 4e-4, "hjm zero price from a flat curve must equal exp(-r0 T)");
      }
    }
    {
      bool refused = false;
      try {
        (void)rate::hjm_paths(8, 8, 0.5, hjm, rate::SimOptions{}, cfg, 1);
      } catch (const std::exception&) {
        refused = true;
      }
      expect(refused, "hjm_paths must refuse a step larger than the smallest tenor spacing");
    }

    ropt.paths_first = true;
    const auto hjm_paths_pf = rate::hjm_paths(64, 128, 1.0 / 252.0, hjm, ropt, cfg, ropt.seed);
    expect(hjm_paths_pf.rate.sizes()[0] == 128 && hjm_paths_pf.rate.sizes()[1] == 65,
           "hjm paths_first rate shape mismatch");
    expect(hjm_paths_pf.forward.sizes()[0] == 128 && hjm_paths_pf.forward.sizes()[1] == 65 &&
               hjm_paths_pf.forward.sizes()[2] == static_cast<std::int64_t>(hjm.tenors.size()),
           "hjm paths_first forward shape mismatch");

    test_hull_white_transition_is_exact_for_a_sloping_level();
    test_zero_coupon_prices_match_simulation();
    test_quasi_random_rate_paths_are_accurate();
    std::cout << "rates_paths_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "rates_paths_test failed: " << e.what() << '\n';
    return 1;
  }
}
