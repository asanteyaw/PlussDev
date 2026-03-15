#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::subsp::math::finance::calibration::HestonCalibMethod;
using pluss::subsp::math::finance::calibration::HestonCalibOptions;
using pluss::subsp::math::finance::calibration::OptionQuote;
using pluss::subsp::math::finance::calibration::SvCalibOptions;
using pluss::subsp::math::finance::calibration::calibrate_heston;
using pluss::subsp::math::finance::calibration::calibrate_sv;
using pluss::subsp::math::finance::calibration::heston_sse;
using pluss::subsp::math::finance::calibration::sv_sse;
using pluss::subsp::math::finance::pricing::HestonIntegrationOptions;
using pluss::subsp::math::finance::pricing::MonteCarloOptions;
using pluss::subsp::math::finance::pricing::OptionType;
using pluss::subsp::math::finance::pricing::heston_price;
using pluss::subsp::math::finance::pricing::sv_price_mc;
using pluss::subsp::math::finance::volatility::HestonParams;
using pluss::subsp::math::finance::volatility::SvParams;

void expect(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<OptionQuote> build_heston_quotes(const HestonParams& params,
                                             double spot,
                                             double rate,
                                             double dividend_yield) {
  HestonIntegrationOptions int_opt;
  int_opt.integration_steps = 160;
  int_opt.integration_u_max = 80.0;

  const std::vector<double> maturities{0.75, 1.5};
  const std::vector<double> strikes{90.0, 100.0, 110.0};

  std::vector<OptionQuote> quotes;
  quotes.reserve(maturities.size() * strikes.size());

  for (double t : maturities) {
    for (double k : strikes) {
      OptionQuote q;
      q.type = OptionType::Call;
      q.strike = k;
      q.maturity = t;
      q.market_price = heston_price(OptionType::Call, spot, k, rate, dividend_yield, t, params, int_opt);
      q.weight = 1.0;
      quotes.push_back(q);
    }
  }
  return quotes;
}

std::vector<OptionQuote> build_sv_quotes(const SvParams& params,
                                         const MonteCarloOptions& mc,
                                         double spot,
                                         double rate,
                                         double dividend_yield,
                                         Config cfg) {
  const std::vector<double> maturities{0.75};
  const std::vector<double> strikes{90.0, 100.0, 110.0};

  std::vector<OptionQuote> quotes;
  quotes.reserve(maturities.size() * strikes.size());

  for (double t : maturities) {
    for (double k : strikes) {
      OptionQuote q;
      q.type = OptionType::Call;
      q.strike = k;
      q.maturity = t;
      q.market_price = sv_price_mc(OptionType::Call, spot, k, rate, dividend_yield, t, params, mc, cfg);
      q.weight = 1.0;
      quotes.push_back(q);
    }
  }
  return quotes;
}

void test_heston_calibration_objective_and_fit() {
  const auto cfg = f64_cpu();
  const double spot = 100.0;
  const double rate = 0.015;
  const double q = 0.0;

  HestonParams truth;
  truth.kappa = 1.8;
  truth.theta = 0.05;
  truth.sigma = 0.28;
  truth.rho = -0.65;
  truth.v0 = 0.045;

  const auto quotes = build_heston_quotes(truth, spot, rate, q);

  HestonParams init = truth;
  init.kappa = 3.2;
  init.theta = 0.09;
  init.sigma = 0.60;
  init.rho = -0.15;
  init.v0 = 0.09;

  HestonCalibOptions opt;
  opt.method = HestonCalibMethod::CharFn;
  opt.random_starts = 2;
  opt.max_iterations = 12;
  opt.step_fraction = 0.18;
  opt.step_decay = 0.70;
  opt.tolerance = 1e-4;
  opt.seed = 20260317ULL;
  opt.integration.integration_steps = 160;
  opt.integration.integration_u_max = 80.0;

  const double sse_init = heston_sse(quotes, spot, rate, q, init, opt, cfg);
  const auto fit = calibrate_heston(quotes, spot, rate, q, init, opt, cfg);

  expect(std::isfinite(sse_init), "heston initial SSE should be finite");
  expect(std::isfinite(fit.objective), "heston fitted objective should be finite");
  expect(std::isfinite(fit.rmse), "heston fitted RMSE should be finite");
  expect(fit.objective <= sse_init + 1e-10, "heston calibration should not worsen objective");
  expect(fit.evaluations > 0, "heston calibration should evaluate objective");
}

void test_sv_calibration_objective_and_fit() {
  const auto cfg = f64_cpu();
  const double spot = 100.0;
  const double rate = 0.01;
  const double q = 0.0;

  SvParams truth;
  truth.mu = 0.0;
  truth.logvar_mean = -3.2;
  truth.phi = 0.97;
  truth.vol_of_logvar = 0.24;
  truth.rho = -0.55;
  truth.logvar0 = -3.1;

  MonteCarloOptions mc;
  mc.paths = 1200;
  mc.steps = 40;
  mc.seed = 20260318ULL;

  const auto quotes = build_sv_quotes(truth, mc, spot, rate, q, cfg);

  SvParams init = truth;
  init.logvar_mean = -2.5;
  init.phi = 0.80;
  init.vol_of_logvar = 0.45;
  init.rho = -0.10;
  init.logvar0 = -2.7;

  SvCalibOptions opt;
  opt.random_starts = 2;
  opt.max_iterations = 8;
  opt.step_fraction = 0.20;
  opt.step_decay = 0.68;
  opt.tolerance = 1e-3;
  opt.seed = 20260319ULL;
  opt.mc = mc;

  const double sse_init = sv_sse(quotes, spot, rate, q, init, opt, cfg);
  const auto fit = calibrate_sv(quotes, spot, rate, q, init, opt, cfg);

  expect(std::isfinite(sse_init), "sv initial SSE should be finite");
  expect(std::isfinite(fit.objective), "sv fitted objective should be finite");
  expect(std::isfinite(fit.rmse), "sv fitted RMSE should be finite");
  expect(fit.objective <= sse_init + 1e-10, "sv calibration should not worsen objective");
  expect(fit.evaluations > 0, "sv calibration should evaluate objective");
}

} // namespace

int main() {
  try {
    test_heston_calibration_objective_and_fit();
    test_sv_calibration_objective_and_fit();
    std::cout << "heston_sv_calibration_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "heston_sv_calibration_test failed: " << e.what() << '\n';
    return 1;
  }
}
