// Price an Asian call option by simulation: draw many price paths at once, average each path, discount, and report the error.
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

int main() {
  using pluss::ScalarType;
  const double spot = 100.0, strike = 100.0, rate = 0.03, vol = 0.2, years = 1.0;
  const std::int64_t paths = 50000, steps = 52;
  const double dt = years / steps;

  // 1. All shocks in one array: one row per path, one column per week.
  auto z = pluss::series::randn({paths, steps}, ScalarType::Float64, 2024);

  // 2. Geometric Brownian motion: each step multiplies the price by exp((r - vol^2 / 2) dt + vol sqrt(dt) z).
  auto log_step = z * (vol * std::sqrt(dt)) + (rate - 0.5 * vol * vol) * dt;
  auto price = log_step.cumsum(1).exp() * spot;
  std::cout << "paths " << price.size(0) << ", steps " << price.size(1) << "\n";
  std::cout << "mean final price " << price.select(1, steps - 1).mean().item<double>() << " (forward " << spot * std::exp(rate * years) << ")\n";

  // 3. Payoff of an Asian call: the average price over the path against the strike.
  auto payoff = (price.mean(1) - strike).clamp_min(0.0);
  auto discounted = payoff * std::exp(-rate * years);

  // 4. The estimate and its Monte Carlo standard error.
  const double estimate = discounted.mean().item<double>();
  const double stderror = discounted.std().item<double>() / std::sqrt(static_cast<double>(paths));
  std::cout << "price " << estimate << " +/- " << stderror << " (one standard error)\n";
  std::cout << "paths ending in the money: " << discounted.gt(0.0).sum().item<double>() / paths << "\n";
  std::cout << "95th percentile of the payoff " << discounted.quantile(0.95).item<double>() << "\n";
}
