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

  const DeltaHedgeResult out = hedging::delta_hedge(spot, OptionType::Call, k, r, q, t, sigma, cfg);

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

} // namespace

int main() {
  try {
    test_delta_hedge_shapes_and_sanity();
    std::cout << "delta_hedge_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "delta_hedge_test failed: " << e.what() << '\n';
    return 1;
  }
}
