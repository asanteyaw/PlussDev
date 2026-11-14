#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fin = pluss::subsp::math::finance;
using fin::volatility::Ngarch11Params;
using pluss::Series;

void expect(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

pluss::Config f64_cpu() {
  return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
}

// daily log-returns of the NGARCH-in-mean model: R = r + lambda sqrt(h) - h / 2 + sqrt(h) z, h' = omega + beta h + alpha h (z + gamma)^2
Series simulate_ngarch(const Ngarch11Params& p, std::int64_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::normal_distribution<double> normal(0.0, 1.0);
  std::vector<double> out(static_cast<std::size_t>(n));
  double h = fin::volatility::unconditional_variance(p);
  for (std::int64_t t = 0; t < n; ++t) {
    const double z = normal(rng);
    out[static_cast<std::size_t>(t)] = p.r + p.lambda * std::sqrt(h) - 0.5 * h + std::sqrt(h) * z;
    h = p.omega + p.beta * h + p.alpha * h * (z + p.gamma) * (z + p.gamma);
  }
  return pluss::make_series(out, {n}, f64_cpu());
}

Ngarch11Params params() {
  Ngarch11Params p;
  p.omega = 2e-6;
  p.alpha = 0.06;
  p.beta = 0.90;
  p.gamma = 0.5;
  p.lambda = 0.05;
  p.r = 0.0001;
  return p;
}

void test_mean_has_the_convexity_correction() {
  const auto p = params();
  const Series y = simulate_ngarch(p, 500, 1ULL);
  const auto metrics = fin::pricing::pricing_metrics(y, p, f64_cpu());
  const Series expected = metrics.variance.sqrt().mul(p.lambda).add(p.r).sub(metrics.variance.mul(0.5));
  expect(metrics.mean.sub(expected).abs().max().item<double>() < 1e-14, "the conditional mean must be r + lambda sqrt(h) - h / 2");
  expect(std::abs(metrics.loglik - metrics.loglik_per_obs.sum().item<double>()) < 1e-9, "loglik must be the sum of the per-observation terms");
  expect(metrics.standardized_innovation.abs().max().item<double>() < 8.0, "standardised innovations must be moderate");
}

void test_likelihood_identifies_the_risk_premium() {
  const auto p = params();
  const Series y = simulate_ngarch(p, 6000, 7ULL);
  double best_lambda = 0.0, best = -1e300;
  for (const double lambda : {-0.10, 0.0, 0.05, 0.10, 0.20}) {
    auto q = p;
    q.lambda = lambda;
    const double ll = fin::pricing::pricing_metrics(y, q, f64_cpu()).loglik;
    if (ll > best) {
      best = ll;
      best_lambda = lambda;
    }
  }
  expect(std::abs(best_lambda - 0.05) < 1e-12 || std::abs(best_lambda - 0.10) < 1e-12 || std::abs(best_lambda - 0.0) < 1e-12, "the likelihood must peak near the true risk premium");
  auto wrong = p;
  wrong.lambda = 0.5;
  expect(fin::pricing::pricing_metrics(y, wrong, f64_cpu()).loglik < fin::pricing::pricing_metrics(y, p, f64_cpu()).loglik, "a far-off risk premium must lower the likelihood");
}

void test_forecast_and_unconditional_variance() {
  const auto p = params();
  const double unconditional = fin::volatility::unconditional_variance(p);
  expect(std::abs(unconditional - p.omega / (1.0 - p.beta - p.alpha * (1.0 + p.gamma * p.gamma))) < 1e-15, "unconditional variance formula");
  const Series y = simulate_ngarch(p, 800, 3ULL);
  const auto forecast = fin::pricing::pricing_variance_forecast(400, y, p, f64_cpu()).items<double>();
  expect(forecast.size() == 400, "forecast length");
  expect(std::abs(forecast.back() - unconditional) < 0.05 * unconditional, "the forecast must converge to the unconditional variance");
  const double persistence = p.beta + p.alpha * (1.0 + p.gamma * p.gamma);
  expect(std::abs((forecast[10] - unconditional) / (forecast[9] - unconditional) - persistence) < 1e-9, "the forecast must revert at the rate of the persistence");
}

void test_joint_likelihood_weights() {
  const fin::bridge::JointLikelihoodWeights w{.returns_weight = 2.0, .options_weight = 0.5};
  expect(std::abs(fin::bridge::weighted_joint_loglik(10.0, -4.0, w) - 18.0) < 1e-15, "weighted joint loglik");
  expect(std::abs(fin::bridge::weighted_joint_nll(10.0, -4.0, w) + 18.0) < 1e-15, "weighted joint nll");
  bool threw = false;
  try {
    (void)fin::bridge::weighted_joint_loglik(1.0, 1.0, {.returns_weight = -1.0});
  } catch (const std::exception&) {
    threw = true;
  }
  expect(threw, "negative weights must be rejected");
}

} // namespace

int main() {
  try {
    test_mean_has_the_convexity_correction();
    test_likelihood_identifies_the_risk_premium();
    test_forecast_and_unconditional_variance();
    test_joint_likelihood_weights();
    std::cout << "vol_bridge_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "vol_bridge_test failed: " << e.what() << '\n';
    return 1;
  }
}
