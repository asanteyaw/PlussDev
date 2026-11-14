#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::finance::hedging::DeltaHedgeResult;
using pluss::subsp::math::finance::pricing::OptionType;
namespace hedging = pluss::subsp::math::finance::hedging;

void expect(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series make_gbm_path_(std::int64_t n, double s0, double mu, double sigma, double t, std::uint64_t seed, Config cfg) {
  std::vector<double> out(static_cast<std::size_t>(n), s0);
  std::mt19937_64 rng(seed);
  std::normal_distribution<double> zdist(0.0, 1.0);
  const double dt = t / static_cast<double>(n - 1);
  const double drift = (mu - 0.5 * sigma * sigma) * dt;
  const double vol = sigma * std::sqrt(dt);
  for (std::int64_t i = 1; i < n; ++i) {
    const double z = zdist(rng);
    out[static_cast<std::size_t>(i)] = out[static_cast<std::size_t>(i - 1)] * std::exp(drift + vol * z);
  }
  return pluss::make_series(out, {n}, ScalarType::Float64).to(cfg.platform);
}

void test_delta_hedge_shapes_and_sanity() {
  const auto cfg = f64_cpu();
  const std::int64_t n = 252;
  const double t = 1.0;
  const double r = 0.02;
  const double q = 0.0;
  const double sigma = 0.2;
  const double k = 100.0;
  const Series spot = make_gbm_path_(n, 100.0, r, sigma, t, 20260311ULL, cfg);

  const DeltaHedgeResult out = hedging::detail::delta_hedge(spot, OptionType::Call, k, r, q, t, sigma, cfg);

  expect(out.option_value.sizes().size() == 1 && out.option_value.sizes()[0] == n, "option_value shape mismatch");
  expect(out.delta.sizes().size() == 1 && out.delta.sizes()[0] == n, "delta shape mismatch");
  expect(out.cash.sizes().size() == 1 && out.cash.sizes()[0] == n, "cash shape mismatch");
  expect(out.portfolio.sizes().size() == 1 && out.portfolio.sizes()[0] == n, "portfolio shape mismatch");
  expect(out.hedge_error.sizes().size() == 1 && out.hedge_error.sizes()[0] == n, "hedge_error shape mismatch");

  expect(out.option_value.isfinite().all().item<double>() == 1.0, "option_value should be finite");
  expect(out.delta.isfinite().all().item<double>() == 1.0, "delta should be finite");
  expect(out.cash.isfinite().all().item<double>() == 1.0, "cash should be finite");
  expect(out.portfolio.isfinite().all().item<double>() == 1.0, "portfolio should be finite");
  expect(out.hedge_error.isfinite().all().item<double>() == 1.0, "hedge_error should be finite");
  expect(std::isfinite(out.terminal_pnl), "terminal_pnl should be finite");

  const double terminal_from_series = out.hedge_error.select(0, n - 1).item<double>();
  expect(std::abs(out.terminal_pnl - terminal_from_series) <= 1e-12, "terminal_pnl mismatch");
  expect(std::abs(out.terminal_pnl) < 50.0, "terminal_pnl sanity bound breached");
}

// A hedge priced with the model's own volatility replicates the option on average: the mean terminal error is zero, with or without dividends, and its
// spread shrinks like 1 / sqrt(steps). A hedger who ignored the dividends earned on the stock position would lose q * int S Delta dt (about 1.5 here).
void test_delta_hedge_is_unbiased_with_dividends() {
  const auto cfg = f64_cpu();
  const double r = 0.02, q = 0.03, sigma = 0.2, k = 100.0, t = 1.0;
  const std::int64_t paths = 1200;
  double spread[2] = {0.0, 0.0};
  int slot = 0;
  for (const std::int64_t n : {51, 201}) {
    double sum = 0.0, sum2 = 0.0;
    for (std::int64_t i = 0; i < paths; ++i) {
      const Series spot = make_gbm_path_(n, 100.0, r - q, sigma, t, 1000ULL + static_cast<std::uint64_t>(i), cfg);
      const double pnl = hedging::detail::delta_hedge(spot, OptionType::Call, k, r, q, t, sigma, cfg).terminal_pnl;
      sum += pnl;
      sum2 += pnl * pnl;
    }
    const double mean = sum / static_cast<double>(paths);
    spread[slot] = std::sqrt(sum2 / static_cast<double>(paths) - mean * mean);
    expect(std::abs(mean) < 4.0 * spread[slot] / std::sqrt(static_cast<double>(paths)), "delta hedge with dividends must be unbiased");
    ++slot;
  }
  expect(spread[1] < 0.65 * spread[0], "hedging error must shrink as the rebalancing gets finer");
}

// The field-based hedge of a Black-Scholes field is the Black-Scholes hedge; a Heston field (bump-and-reprice delta) must give a finite, near-replicating hedge.
void test_field_delta_hedge() {
  namespace fin = pluss::subsp::math::finance;
  const auto cfg = f64_cpu();
  const std::int64_t n = 101;
  const double r = 0.02, q = 0.01, sigma = 0.2, k = 100.0, t = 1.0;
  const Series spot = make_gbm_path_(n, 100.0, r, sigma, t, 7ULL, cfg);
  fin::module::Field f{
      .instrument = fin::instrument::Vanilla{.type = OptionType::Call, .strike = pluss::series::full({1}, k, cfg), .maturity = pluss::series::full({1}, t, cfg)},
      .market = fin::market::State{.spot = pluss::series::full({1}, 100.0, cfg), .rate = pluss::series::full({1}, r, cfg), .dividend_yield = pluss::series::full({1}, q, cfg)},
      .model = fin::model::BlackScholes{.volatility = pluss::series::full({1}, sigma, cfg)},
      .controls = fin::module::Controls{.engine = fin::module::Engine::Analytic},
  };
  const auto viaField = hedging::delta_hedge(f, spot, cfg);
  const auto direct = hedging::detail::delta_hedge(spot, OptionType::Call, k, r, q, t, sigma, cfg);
  expect(std::abs(viaField.terminal_pnl - direct.terminal_pnl) < 1e-6, "field hedge must match the Black-Scholes hedge");
  expect((viaField.delta - direct.delta).abs().max().item<double>() < 1e-8, "field delta must match the closed form");

  f.model = fin::model::Heston{.kappa = pluss::series::full({1}, 1.5, cfg), .theta = pluss::series::full({1}, 0.04, cfg),
                               .sigma = pluss::series::full({1}, 0.30, cfg), .rho = pluss::series::full({1}, -0.70, cfg),
                               .v0 = pluss::series::full({1}, 0.04, cfg)};
  f.controls.engine = fin::module::Engine::Cos;
  const auto heston = hedging::delta_hedge(f, spot, cfg);
  expect(heston.hedge_error.isfinite().all().item<double>() == 1.0 && std::abs(heston.terminal_pnl) < 15.0, "heston field hedge must be finite and bounded");
}

} // namespace

int main() {
  try {
    test_delta_hedge_shapes_and_sanity();
    test_delta_hedge_is_unbiased_with_dividends();
    test_field_delta_hedge();
    std::cout << "delta_hedge_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "delta_hedge_test failed: " << e.what() << '\n';
    return 1;
  }
}
