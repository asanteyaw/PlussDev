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
using pluss::subsp::math::finance::pricing::BlackScholesSeriesGreeks;
using pluss::subsp::math::finance::pricing::ImpliedVolMethod;
using pluss::subsp::math::finance::pricing::ImpliedVolOptions;
using pluss::subsp::math::finance::pricing::OptionType;
namespace pricing = pluss::subsp::math::finance::pricing;

void expect(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

void test_scalar_black_scholes_reference() {
  constexpr double s = 100.0;
  constexpr double k = 100.0;
  constexpr double r = 0.05;
  constexpr double q = 0.0;
  constexpr double t = 1.0;
  constexpr double v = 0.2;

  const double call = pricing::black_scholes_price(OptionType::Call, s, k, r, q, t, v);
  const double put = pricing::black_scholes_price(OptionType::Put, s, k, r, q, t, v);
  const auto g_call = pricing::black_scholes_greeks(OptionType::Call, s, k, r, q, t, v);

  expect(approx(call, 10.4505835722, 1e-6), "call price reference mismatch");
  expect(approx(put, 5.5735260223, 1e-6), "put price reference mismatch");
  expect(approx(g_call.delta, 0.6368306512, 1e-6), "call delta reference mismatch");
  expect(approx(g_call.gamma, 0.0187620173, 1e-6), "gamma reference mismatch");
  expect(approx(g_call.vega, 37.52403469, 1e-5), "vega reference mismatch");
  expect(approx(g_call.theta, -6.414027546, 1e-5), "theta reference mismatch");
  expect(approx(g_call.rho, 53.232481545, 1e-5), "rho reference mismatch");

  const double parity_lhs = call - put;
  const double parity_rhs = s * std::exp(-q * t) - k * std::exp(-r * t);
  expect(approx(parity_lhs, parity_rhs, 1e-6), "put-call parity mismatch");
}

void test_series_black_scholes_ttm() {
  const auto cfg = f64_cpu();
  const Series s = pluss::make_series(std::vector<double>{90.0, 100.0, 110.0}, {3}, cfg);
  const Series tau = pluss::make_series(std::vector<double>{1.0, 0.5, 0.0}, {3}, cfg);

  const Series call = pricing::black_scholes_price(OptionType::Call, s, tau, 100.0, 0.03, 0.01, 0.25, cfg);
  const BlackScholesSeriesGreeks g =
      pricing::black_scholes_greeks(OptionType::Call, s, tau, 100.0, 0.03, 0.01, 0.25, cfg);

  expect(call.sizes().size() == 1 && call.sizes()[0] == 3, "series call output shape mismatch");
  expect(g.delta.sizes().size() == 1 && g.delta.sizes()[0] == 3, "series delta output shape mismatch");
  expect(call.isfinite().all().item<double>() == 1.0, "series call contains non-finite values");
  expect(g.delta.isfinite().all().item<double>() == 1.0, "series delta contains non-finite values");

  const double maturity_price = call.select(0, 2).item<double>();
  expect(approx(maturity_price, 10.0, 1e-12), "maturity price should match intrinsic value");

  const double maturity_gamma = g.gamma.select(0, 2).item<double>();
  const double maturity_vega = g.vega.select(0, 2).item<double>();
  expect(approx(maturity_gamma, 0.0, 1e-12), "maturity gamma should be zero");
  expect(approx(maturity_vega, 0.0, 1e-12), "maturity vega should be zero");
}

void test_implied_vol_scalar_methods() {
  constexpr double s = 100.0;
  constexpr double k = 100.0;
  constexpr double r = 0.05;
  constexpr double q = 0.01;
  constexpr double t = 0.75;
  constexpr double sigma_true = 0.28;

  const double call_px = pricing::black_scholes_price(OptionType::Call, s, k, r, q, t, sigma_true);
  const double put_px = pricing::black_scholes_price(OptionType::Put, s, k, r, q, t, sigma_true);

  const std::vector<ImpliedVolMethod> methods = {
      ImpliedVolMethod::Auto,
      ImpliedVolMethod::Newton,
      ImpliedVolMethod::Bisection,
      ImpliedVolMethod::Secant,
      ImpliedVolMethod::Brent,
  };
  for (const auto m : methods) {
    ImpliedVolOptions opt;
    opt.method = m;
    opt.tolerance = 1e-10;
    opt.max_iterations = 256;
    const double c_iv = pricing::implied_vol(OptionType::Call, call_px, s, k, r, q, t, opt);
    const double p_iv = pricing::implied_vol(OptionType::Put, put_px, s, k, r, q, t, opt);
    expect(std::isfinite(c_iv), "scalar implied vol call should be finite");
    expect(std::isfinite(p_iv), "scalar implied vol put should be finite");
    expect(std::abs(c_iv - sigma_true) <= 1e-7, "scalar implied vol call mismatch");
    expect(std::abs(p_iv - sigma_true) <= 1e-7, "scalar implied vol put mismatch");
  }
}

void test_implied_vol_series() {
  const auto cfg = f64_cpu();
  const Series s = pluss::make_series(std::vector<double>{90.0, 95.0, 100.0, 105.0, 110.0}, {5}, cfg);
  const Series tau = pluss::make_series(std::vector<double>{1.00, 0.80, 0.60, 0.40, 0.20}, {5}, cfg);
  constexpr double k = 100.0;
  constexpr double r = 0.03;
  constexpr double q = 0.01;
  constexpr double sigma_true = 0.22;

  const Series px = pricing::black_scholes_price(OptionType::Call, s, tau, k, r, q, sigma_true, cfg);
  ImpliedVolOptions opt;
  opt.method = ImpliedVolMethod::Brent;
  opt.tolerance = 1e-10;
  opt.max_iterations = 256;
  const Series iv = pricing::implied_vol(OptionType::Call, px, s, tau, k, r, q, opt, cfg);

  expect(iv.sizes().size() == 1 && iv.sizes()[0] == 5, "series implied vol shape mismatch");
  expect(iv.isfinite().all().item<double>() == 1.0, "series implied vol should be finite");
  const double max_abs_err = iv.sub(sigma_true).abs().max().item<double>();
  expect(max_abs_err <= 1e-7, "series implied vol max error mismatch");
}

void test_implied_vol_no_solution_nan() {
  ImpliedVolOptions opt;
  opt.method = ImpliedVolMethod::Brent;
  const double iv_bad_low = pricing::implied_vol(OptionType::Call, -1.0, 100.0, 100.0, 0.03, 0.0, 1.0, opt);
  const double iv_bad_high = pricing::implied_vol(OptionType::Call, 200.0, 100.0, 100.0, 0.03, 0.0, 1.0, opt);
  const double iv_bad_t = pricing::implied_vol(OptionType::Call, 10.0, 100.0, 100.0, 0.03, 0.0, 0.0, opt);
  expect(std::isnan(iv_bad_low), "invalid low price should return NaN");
  expect(std::isnan(iv_bad_high), "invalid high price should return NaN");
  expect(std::isnan(iv_bad_t), "non-positive maturity should return NaN");
}

} // namespace

int main() {
  try {
    test_scalar_black_scholes_reference();
    test_series_black_scholes_ttm();
    test_implied_vol_scalar_methods();
    test_implied_vol_series();
    test_implied_vol_no_solution_nan();
    std::cout << "black_scholes_test passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "black_scholes_test failed: " << e.what() << '\n';
    return 1;
  }
}
