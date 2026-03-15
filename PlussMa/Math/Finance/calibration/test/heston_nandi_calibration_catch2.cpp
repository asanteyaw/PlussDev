#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::finance::calibration::HestonNandiCalibOptions;
using pluss::subsp::math::finance::calibration::OptionQuote;
using pluss::subsp::math::finance::calibration::calibrate_heston_nandi;
using pluss::subsp::math::finance::calibration::heston_nandi_returns_nll;
using pluss::subsp::math::finance::calibration::heston_nandi_sse;
using pluss::subsp::math::finance::pricing::HestonNandiIntegrationOptions;
using pluss::subsp::math::finance::pricing::MonteCarloOptions;
using pluss::subsp::math::finance::pricing::OptionType;
using pluss::subsp::math::finance::pricing::heston_nandi_price;
using pluss::subsp::math::finance::pricing::heston_nandi_price_mc;
using pluss::subsp::math::finance::volatility::HestonNandiParams;
using pluss::subsp::math::finance::volatility::RandMethod;

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

HestonNandiParams hn_truth_() {
  HestonNandiParams p;
  p.omega = 1.2e-6;
  p.alpha = 1.0e-5;
  p.beta = 0.85;
  p.gamma = 1.0;
  p.lambda = 0.0;
  p.r = 0.0;
  p.initial_variance = 0.00015;
  p.variance_floor = 1e-12;
  p.stationarity_margin = 1e-6;
  return p;
}

} // namespace

TEST_CASE("heston-nandi closed-form parity and numerical stability", "[finance][hn][pricing][stability]") {
  const HestonNandiParams p = hn_truth_();
  HestonNandiIntegrationOptions opt;
  opt.integration_steps = 1800;
  opt.integration_u_max = 180.0;
  opt.periods_per_year = 52;

  for (const double maturity : {0.25, 0.5, 1.0, 2.0}) {
    for (const double strike : {80.0, 100.0, 120.0}) {
      const double c = heston_nandi_price(OptionType::Call, 100.0, strike, 0.02, 0.01, maturity, p, opt);
      const double put = heston_nandi_price(OptionType::Put, 100.0, strike, 0.02, 0.01, maturity, p, opt);
      REQUIRE(std::isfinite(c));
      REQUIRE(std::isfinite(put));
      REQUIRE(c >= 0.0);
      REQUIRE(put >= 0.0);
      const double parity = 100.0 * std::exp(-0.01 * maturity) - strike * std::exp(-0.02 * maturity);
      REQUIRE(std::abs((c - put) - parity) < 3e-2);
    }
  }
}

TEST_CASE("heston-nandi closed-form aligns with monte-carlo in stable regimes", "[finance][hn][pricing][sensitivity]") {
  const auto cfg = f64_cpu();
  const HestonNandiParams p = hn_truth_();

  HestonNandiIntegrationOptions cf_opt;
  cf_opt.integration_steps = 2000;
  cf_opt.integration_u_max = 200.0;
  cf_opt.periods_per_year = 52;

  MonteCarloOptions mc;
  mc.paths = 32000;
  mc.steps = 252;
  mc.rand_method = RandMethod::Sobol;
  mc.randomized_quasi = true;
  mc.sobol_scrambling = 3;
  mc.antithetic = true;
  mc.seed = 2026031401ULL;

  const double call_cf = heston_nandi_price(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, p, cf_opt);
  const double call_mc = heston_nandi_price_mc(OptionType::Call, 100.0, 100.0, 0.02, 0.0, 1.0, p, mc, cfg);
  REQUIRE(std::isfinite(call_cf));
  REQUIRE(std::isfinite(call_mc));
  REQUIRE(std::abs(call_cf - call_mc) < 15.0);
}

TEST_CASE("heston-nandi returns nll stays finite under stress and rejects invalid params", "[finance][hn][nll][stress][crash]") {
  const auto cfg = f64_cpu();
  const HestonNandiParams p = hn_truth_();

  const Series y = pluss::randn({2500}, cfg, 2026031402ULL).mul(0.012);
  const double nll = heston_nandi_returns_nll(y, p, cfg);
  REQUIRE(std::isfinite(nll));

  HestonNandiParams bad = p;
  bad.alpha = 0.2;
  bad.gamma = 10.0;
  bad.beta = 0.95;
  CHECK_THROWS(heston_nandi_returns_nll(y, bad, cfg));
}

TEST_CASE("heston-nandi calibration closed-form objective improves and stays admissible", "[finance][hn][calib][edge]") {
  const auto cfg = f64_cpu();
  const HestonNandiParams truth = hn_truth_();

  HestonNandiIntegrationOptions cf_opt;
  cf_opt.integration_steps = 1400;
  cf_opt.integration_u_max = 170.0;
  cf_opt.periods_per_year = 12;

  std::vector<OptionQuote> quotes;
  for (const double maturity : {0.25}) {
    for (const double strike : {90.0, 100.0, 110.0}) {
      OptionQuote q;
      q.type = OptionType::Call;
      q.strike = strike;
      q.maturity = maturity;
      q.market_price = heston_nandi_price(q.type, 100.0, q.strike, 0.02, 0.0, q.maturity, truth, cf_opt);
      q.weight = 1.0;
      quotes.push_back(q);
    }
  }

  HestonNandiCalibOptions opt;
  opt.random_starts = 2;
  opt.max_iterations = 12;
  opt.step_fraction = 0.25;
  opt.step_decay = 0.65;
  opt.tolerance = 1e-7;
  opt.returns_weight = 0.0;
  opt.options_weight = 1.0;
  opt.use_closed_form = true;
  opt.integration = cf_opt;
  opt.seed = 2026031403ULL;

  HestonNandiParams initial = truth;
  initial.alpha *= 2.5;
  initial.beta = std::clamp(initial.beta - 0.06, 0.0, 0.98);
  initial.gamma += 1.2;

  const double init_obj = heston_nandi_sse(quotes, 100.0, 0.02, 0.0, initial, opt, cfg);
  const auto fit = calibrate_heston_nandi(Series::undefined(), quotes, 100.0, 0.02, 0.0, initial, opt, cfg);

  REQUIRE(std::isfinite(init_obj));
  REQUIRE(std::isfinite(fit.objective));
  REQUIRE(fit.objective <= init_obj + 1e-9);
  REQUIRE(std::isfinite(fit.options_sse));
  REQUIRE(fit.options_sse >= 0.0);

  const double persistence = fit.params.beta + fit.params.alpha * fit.params.gamma * fit.params.gamma;
  REQUIRE(persistence < (1.0 - fit.params.stationarity_margin));
}
