#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
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
using pluss::subsp::math::finance::pricing::OptionType;
using pluss::subsp::math::finance::pricing::heston_nandi_price;
using pluss::subsp::math::finance::volatility::HestonNandiParams;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_heston_nandi_calibration_lane() {
  const auto cfg = f64_cpu();

  HestonNandiParams truth;
  truth.omega = 1.2e-6;
  truth.alpha = 2.0e-5;
  truth.beta = 0.92;
  truth.gamma = 5.0;
  truth.lambda = 0.0;
  truth.r = 0.0;
  truth.initial_variance = 0.00015;

  const Series returns = pluss::randn({600}, cfg, 20260331ULL).mul(0.012);
  const double nll = heston_nandi_returns_nll(returns, truth, cfg);
  expect(std::isfinite(nll), "heston_nandi_returns_nll should be finite");

  HestonNandiIntegrationOptions cf_opt;
  cf_opt.integration_steps = 1600;
  cf_opt.integration_u_max = 180.0;
  cf_opt.periods_per_year = 252;

  std::vector<OptionQuote> quotes;
  for (double k : {90.0, 100.0, 110.0}) {
    OptionQuote q;
    q.type = OptionType::Call;
    q.strike = k;
    q.maturity = 1.0;
    q.market_price = heston_nandi_price(q.type, 100.0, q.strike, 0.02, 0.0, q.maturity, truth, cf_opt);
    q.weight = 1.0;
    quotes.push_back(q);
  }

  HestonNandiCalibOptions opt;
  opt.random_starts = 2;
  opt.max_iterations = 10;
  opt.returns_weight = 1.0;
  opt.options_weight = 1.0;
  opt.use_closed_form = true;
  opt.integration = cf_opt;
  opt.seed = 20260402ULL;

  const double sse = heston_nandi_sse(quotes, 100.0, 0.02, 0.0, truth, opt, cfg);
  expect(std::isfinite(sse) && sse >= 0.0, "heston_nandi_sse should be finite and non-negative");

  HestonNandiParams initial = truth;
  initial.alpha = 0.20;
  initial.beta = 0.70;
  initial.gamma = 0.1;

  const auto fit = calibrate_heston_nandi(returns, quotes, 100.0, 0.02, 0.0, initial, opt, cfg);
  expect(std::isfinite(fit.objective), "calibrate_heston_nandi objective should be finite");
  expect(std::isfinite(fit.returns_nll), "calibrate_heston_nandi returns_nll should be finite");
  expect(std::isfinite(fit.options_sse), "calibrate_heston_nandi options_sse should be finite");
}

} // namespace

int main() {
  try {
    test_heston_nandi_calibration_lane();
    std::cout << "heston_nandi_calibration_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "heston_nandi_calibration_test failed: " << e.what() << '\n';
    return 1;
  }
}
