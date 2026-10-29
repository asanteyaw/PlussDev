#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace prc = pluss::subsp::math::finance::pricing;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

} // namespace

int main() {
  try {
    const double rate = 0.03, dividend = 0.01;

    // Every method recovers the volatility of an ordinary price, a deep out-of-the-money price (where the absolute price tolerance used to
    // accept every volatility below a threshold) and a deep in-the-money price.
    struct Case {
      prc::OptionType type;
      double spot, strike, maturity, vol;
    };
    const std::vector<Case> cases{{prc::Call, 100, 100, 1.0, 0.3},
                                  {prc::Call, 100, 160, 0.04, 0.3},
                                  {prc::Call, 100, 40, 1.0, 0.3},
                                  {prc::Put, 100, 70, 0.5, 0.45},
                                  {prc::Put, 100, 60, 0.1, 0.12}};
    for (const auto& c : cases) {
      const double price = prc::black_scholes_price(c.type, c.spot, c.strike, rate, dividend, c.maturity, c.vol);
      for (auto method : {prc::Auto, prc::Newton, prc::Bisection, prc::Secant, prc::Brent}) {
        prc::ImpliedVolOptions options;
        options.method = method;
        const double iv = prc::implied_vol(c.type, price, c.spot, c.strike, rate, dividend, c.maturity, options);
        expect(std::abs(iv - c.vol) < 1e-6, "implied vol recovers the volatility of the price for every method");
      }
    }

    // Brent's method converges superlinearly: it needs a handful of iterations, far fewer than bisection.
    const double price = prc::black_scholes_price(prc::Call, 100.0, 110.0, rate, 0.0, 0.5, 0.35);
    auto iterations_needed = [&](prc::ImpliedVolMethod method) {
      for (int limit = 1; limit <= 200; ++limit) {
        prc::ImpliedVolOptions options;
        options.method = method;
        options.max_iterations = limit;
        if (std::isfinite(prc::implied_vol(prc::Call, price, 100.0, 110.0, rate, 0.0, 0.5, options))) {
          return limit;
        }
      }
      return 201;
    };
    expect(iterations_needed(prc::Brent) <= 10, "Brent converges in a handful of iterations");
    expect(iterations_needed(prc::Newton) <= 10, "Newton converges in a handful of iterations");
    expect(iterations_needed(prc::Bisection) >= 20, "bisection needs many iterations");

    // A solver that is cut off before converging returns NaN, never its last iterate.
    prc::ImpliedVolOptions one;
    one.method = prc::Bisection;
    one.max_iterations = 1;
    expect(std::isnan(prc::implied_vol(prc::Call, price, 100.0, 110.0, rate, 0.0, 0.5, one)), "an unconverged bisection is NaN");

    // No-arbitrage violations and invalid inputs are NaN.
    expect(std::isnan(prc::implied_vol(prc::Call, 1.0, 100.0, 100.0, rate, 0.0, 1.0)), "below the intrinsic value is NaN");
    expect(std::isnan(prc::implied_vol(prc::Call, 120.0, 100.0, 100.0, rate, 0.0, 1.0)), "above the spot is NaN");
    expect(std::isnan(prc::implied_vol(prc::Call, 9.0, 100.0, 100.0, rate, 0.0, 0.0)), "zero maturity is NaN");

    std::cout << "implied_vol_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "implied_vol_test failed: " << e.what() << "\n";
    return 1;
  }
}
