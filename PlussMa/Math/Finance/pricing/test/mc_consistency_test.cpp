#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

double mean_terminal(const vol::StochVolPaths& p, std::int64_t steps) {
  return p.spot.select(0, steps).mean().item<double>();
}

} // namespace

int main() {
  try {
    const double spot = 100.0, mu = 0.02, maturity = 1.0, rate = 0.03, dividend = 0.01, strike = 100.0;
    const std::int64_t steps = 100, paths = 100000;
    const double dt = maturity / static_cast<double>(steps);
    const double target = spot * std::exp(mu * maturity);

    // The stochastic-volatility generators are martingales after discounting: E[S_T] = S_0 exp(mu T), for every scheme and for
    // correlated shocks. (The spot step used to average the start and end variances, which added a drift of rho * sigma / 4.)
    for (vol::SimScheme scheme : {vol::SimScheme::Euler, vol::SimScheme::Milstein, vol::SimScheme::QuadExp, vol::SimScheme::Transition}) {
      vol::HestonParams h;
      h.mu = mu;
      h.rho = -0.7;
      h.sigma = 0.3;
      vol::SimOptions options;
      options.scheme = scheme;
      options.seed = 11;
      const double m = mean_terminal(vol::detail::heston_paths(steps, paths, spot, dt, h, options), steps);
      expect(std::abs(m - target) / target < 0.004, "heston_paths must reproduce E[S_T] = S0 exp(mu T)");
    }
    {
      vol::BatesParams b;
      b.mu = mu;
      vol::SimOptions options;
      options.seed = 12;
      const double m = mean_terminal(vol::detail::bates_paths(steps, paths, spot, dt, b, options), steps);
      expect(std::abs(m - target) / target < 0.004, "bates_paths must reproduce E[S_T] = S0 exp(mu T) with the jump compensator");
    }
    {
      vol::RoughHestonParams rh;
      rh.mu = mu;
      rh.rho = -0.5;
      const double m = mean_terminal(vol::detail::rough_heston_paths(steps, 20000, spot, dt, rh), steps);
      expect(std::abs(m - target) / target < 0.006, "rough_heston_paths must reproduce E[S_T] = S0 exp(mu T)");
    }

    // Monte Carlo prices agree with the semi-analytic prices to within sampling error.
    prc::MonteCarloOptions mc;
    mc.paths = 200000;
    mc.steps = 100;
    mc.seed = 7;
    vol::HestonParams h;
    const double heston_cos = prc::detail::heston_price_cos(prc::Call, spot, strike, rate, dividend, maturity, h);
    const double heston_mc = prc::detail::heston_price_mc(prc::Call, spot, strike, rate, dividend, maturity, h, mc);
    expect(std::abs(heston_mc - heston_cos) < 0.15, "heston Monte Carlo agrees with the COS price");
    vol::BatesParams b;
    const double bates_cos = prc::detail::bates_price_cos(prc::Call, spot, strike, rate, dividend, maturity, b);
    const double bates_mc = prc::detail::bates_price_mc(prc::Call, spot, strike, rate, dividend, maturity, b, mc);
    expect(std::abs(bates_mc - bates_cos) < 0.2, "bates Monte Carlo agrees with the COS price");

    // An implied volatility that is not found within max_iterations is NaN, not the last iterate.
    const double price = prc::detail::black_scholes_price(prc::Call, spot, strike, rate, dividend, maturity, 0.5);
    prc::ImpliedVolOptions few;
    few.method = prc::ImpliedVolMethod::Newton;
    few.max_iterations = 1;
    expect(std::isnan(prc::detail::implied_vol(prc::Call, price, spot, strike, rate, dividend, maturity, few)), "unconverged implied vol is NaN");
    expect(std::abs(prc::detail::implied_vol(prc::Call, price, spot, strike, rate, dividend, maturity) - 0.5) < 1e-7, "default implied vol converges");

    // Quasi-random sequences have the paths as points and the time steps as dimensions, so they too reproduce E[S_T] = S0 exp(mu T).
    for (vol::RandMethod method : {vol::RandMethod::Sobol, vol::RandMethod::Torus}) {
      vol::HestonParams hq;
      hq.mu = mu;
      vol::SimOptions options;
      options.method = method;
      options.randomized_quasi = true;
      options.seed = 3;
      const double m = mean_terminal(vol::detail::heston_paths(steps, 20000, spot, dt, hq, options), steps);
      expect(std::abs(m - target) / target < 0.01, "quasi-random heston_paths must reproduce E[S_T] = S0 exp(mu T)");
    }
    {
      prc::MonteCarloOptions quasi;
      quasi.paths = 16384;
      quasi.steps = 64;
      quasi.rand_method = vol::RandMethod::Sobol;
      quasi.randomized_quasi = true;
      quasi.seed = 5;
      expect(std::abs(prc::detail::heston_price_mc(prc::Call, spot, strike, rate, dividend, maturity, h, quasi) - heston_cos) < 0.25, "randomized Sobol prices a Heston call");
    }

    // Importance sampling is unbiased: the shift is applied to the spot shock only, and the likelihood ratio accounts for it.
    {
      prc::MonteCarloOptions is;
      is.paths = 20000;
      is.steps = 50;
      is.importance_sampling = true;
      is.importance_shift = 0.05;
      double total = 0.0;
      const int reps = 12;
      for (int rep = 1; rep <= reps; ++rep) {
        is.seed = 1000 + rep;
        total += prc::detail::heston_price_mc(prc::Call, spot, strike, rate, dividend, maturity, h, is);
      }
      expect(std::abs(total / reps - heston_cos) < 0.15, "importance-sampled Monte Carlo is unbiased");
    }

    // Rough Heston at a Hurst exponent next to one half is the ordinary Heston model.
    {
      vol::RoughHestonParams rough;
      rough.alpha = -0.01;
      rough.rho = -0.7;
      vol::HestonParams classic;
      classic.v0 = rough.v0;
      classic.theta = rough.theta;
      classic.kappa = rough.kappa;
      classic.sigma = rough.sigma;
      classic.rho = rough.rho;
      prc::MonteCarloOptions options;
      options.paths = 20000;
      options.steps = 50;
      options.antithetic = true;
      for (double k : {90.0, 100.0, 110.0}) {
        const double reference = prc::detail::heston_price(prc::Call, spot, k, 0.0, 0.0, maturity, classic);
        expect(std::abs(prc::detail::rough_heston_price_mc(prc::Call, spot, k, 0.0, 0.0, maturity, rough, options) - reference) < 0.15,
               "rough_heston_price_mc at H close to one half agrees with the Heston price");
      }
    }

    // The rough Bergomi variance is a martingale started at xi, and the spot a martingale started at S0.
    {
      vol::RoughBergomiParams bergomi;
      bergomi.alpha = -0.35;
      const auto sim = vol::detail::rough_bergomi_paths(50, 40000, spot, 0.02, bergomi);
      const double mean_variance = sim.variance.select(0, 50).mean().item<double>();
      expect(std::abs(mean_variance - bergomi.xi) / bergomi.xi < 0.05, "rough_bergomi_paths variance has mean xi");
      expect(std::abs(sim.spot.select(0, 50).mean().item<double>() - spot) / spot < 0.01, "rough_bergomi_paths spot is a martingale");
    }

    // The generic FFT takes the plain characteristic function of ln S_T, as COS does, and prices the same option.
    {
      const double v = 0.2;
      const auto cf = [&](std::complex<double> u) {
        const std::complex<double> i(0.0, 1.0);
        return std::exp(i * u * (std::log(spot) + (rate - dividend - 0.5 * v * v) * maturity) - 0.5 * v * v * maturity * u * u);
      };
      const double bs = prc::detail::black_scholes_price(prc::Call, spot, strike, rate, dividend, maturity, v);
      expect(std::abs(prc::option_price_fft(prc::Call, spot, strike, rate, dividend, maturity, cf) - bs) < 1e-4, "generic FFT reproduces Black-Scholes");
      expect(std::abs(prc::option_price_cos(prc::Call, spot, strike, rate, dividend, maturity, cf) - bs) < 1e-8, "generic COS reproduces Black-Scholes");
    }

    // The FFT at its default settings is accurate at strikes that are not on its grid, for short, medium and long maturities.
    for (double T : {0.1, 1.0, 5.0}) {
      for (double k : {60.0, 72.0, 85.0, 93.0, 107.0, 118.0, 131.0, 155.0}) {
        const double exact = prc::detail::heston_price_cos(prc::Call, spot, k, rate, dividend, T, h);
        const double fft = prc::detail::heston_price_fft(prc::Call, spot, k, rate, dividend, T, h);
        expect(std::abs(fft - exact) < 5e-5, "FFT prices off-grid strikes at its default settings");
      }
    }

    // With a strongly Feller-violating variance, the exact-transition scheme keeps the correlation with the spot and prices correctly.
    {
      vol::HestonParams rough_variance;
      rough_variance.kappa = 0.5;
      rough_variance.theta = 0.04;
      rough_variance.sigma = 1.0;
      rough_variance.rho = -0.7;
      const double exact = prc::detail::heston_price_cos(prc::Call, spot, strike, rate, dividend, maturity, rough_variance);
      for (vol::SimScheme scheme : {vol::SimScheme::QuadExp, vol::SimScheme::Transition}) {
        prc::MonteCarloOptions o;
        o.paths = 60000;
        o.steps = 32;
        o.scheme = scheme;
        o.seed = 21;
        expect(std::abs(prc::detail::heston_price_mc(prc::Call, spot, strike, rate, dividend, maturity, rough_variance, o) - exact) < 0.3, "QuadExp and Transition price a Feller-violating Heston model");
      }
    }

    // When the model loses the moment the damped transform needs (a large volatility of variance at a long maturity), the FFT lowers its
    // damping exponent instead of returning a wrong price, and agrees with the integration and COS prices.
    {
      vol::HestonParams explosive;
      explosive.v0 = 0.09;
      explosive.theta = 0.06;
      explosive.kappa = 0.8;
      explosive.sigma = 0.9;
      explosive.rho = -0.3;
      for (double k : {80.0, 100.0, 130.0}) {
        const double reference = prc::detail::heston_price(prc::Call, spot, k, rate, dividend, 10.0, explosive);
        expect(std::abs(prc::detail::heston_price_cos(prc::Call, spot, k, rate, dividend, 10.0, explosive) - reference) < 1e-3, "COS prices a model without a finite moment");
        expect(std::abs(prc::detail::heston_price_fft(prc::Call, spot, k, rate, dividend, 10.0, explosive) - reference) < 1e-3, "FFT lowers its damping when the moment does not exist");
      }
    }

    // Input errors name the problem.
    {
      bool steps_rejected = false;
      try {
        prc::HestonIntegrationOptions o;
        o.integration_steps = 1;
        (void)prc::detail::heston_price(prc::Call, spot, strike, rate, dividend, maturity, h, o);
      } catch (const std::exception& e) {
        steps_rejected = std::string(e.what()).find("integration_steps must be >= 2") != std::string::npos;
      }
      expect(steps_rejected, "integration_steps below 2 is rejected with a clear message");
      bool mismatch = false;
      try {
        const auto three = pluss::make_series(std::vector<double>{90, 100, 110}, {3});
        const auto two = pluss::make_series(std::vector<double>{0.5, 1.0}, {2});
        (void)prc::detail::heston_price_cos(prc::Call, three, three, rate, dividend, two, h);
      } catch (const std::exception& e) {
        mismatch = std::string(e.what()).find("must have the same length") != std::string::npos;
      }
      expect(mismatch, "mismatched Series lengths are rejected with a clear message");
    }

    std::cout << "mc_consistency_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "mc_consistency_test failed: " << e.what() << "\n";
    return 1;
  }
}
