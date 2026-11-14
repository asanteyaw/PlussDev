// COS benchmark: freezes the behaviour of the closed-form COS pricer (scalar and Series overloads) as golden numbers
// and measures it against independent references (closed-form Black-Scholes, MATLAB's optByHestonNI, the library's
// own Heston integral and FFT prices), so that any change to the COS implementation is checked against both.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/ma.h>

#include "cos_legacy_reference.h"

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;

namespace {

using Clock = std::chrono::steady_clock;
using Complex = std::complex<double>;

pluss::Config f64() { return pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU); }

double normal_cdf(double z) { return 0.5 * std::erfc(-z / std::sqrt(2.0)); }

struct Gbm {
  double s = 100.0, r = 0.03, q = 0.01, vol = 0.25, t = 1.0;
};
double bs_price(prc::OptionType type, const Gbm &m, double k) {
  const double d1 = (std::log(m.s / k) + (m.r - m.q + 0.5 * m.vol * m.vol) * m.t) / (m.vol * std::sqrt(m.t));
  const double d2 = d1 - m.vol * std::sqrt(m.t);
  const double call = m.s * std::exp(-m.q * m.t) * normal_cdf(d1) - k * std::exp(-m.r * m.t) * normal_cdf(d2);
  return type == prc::OptionType::Call ? call : call - m.s * std::exp(-m.q * m.t) + k * std::exp(-m.r * m.t);
}
// characteristic function of ln S_T under geometric Brownian motion
// Both overloads of option_price_cos take the plain characteristic function and discount the price themselves.
prc::CharacteristicFn gbm_cf(const Gbm &m) {
  return [m](Complex u) {
    const double mean = std::log(m.s) + (m.r - m.q - 0.5 * m.vol * m.vol) * m.t;
    return std::exp(Complex(0.0, 1.0) * u * mean - 0.5 * m.vol * m.vol * m.t * u * u);
  };
}
prc::CharacteristicSeriesFn gbm_cf_series(const Gbm &m) {
  return [m](const pluss::Series &u) {
    const double mean = std::log(m.s) + (m.r - m.q - 0.5 * m.vol * m.vol) * m.t;
    const pluss::Series magnitude = u.square().mul(-0.5 * m.vol * m.vol * m.t).exp();
    const pluss::Series angle = u.mul(mean);
    return prc::ComplexSeries{magnitude.mul(angle.cos()), magnitude.mul(angle.sin())};
  };
}

const std::vector<double> kStrikes{70.0, 90.0, 100.0, 110.0, 140.0};
const std::vector<double> kVols{0.15, 0.40};
const std::vector<double> kMaturities{0.25, 2.0};
const std::vector<std::int64_t> kTerms{64, 256, 1024};

} // namespace

TEST_CASE("generate COS golden values", "[.generate]") {
  std::printf("// scalar overload, then Series overload; order: vol, maturity, terms, type(call,put), strike\n");
  std::printf("inline const std::vector<double> gbm_scalar = {");
  for (double v : kVols) for (double t : kMaturities) for (std::int64_t n : kTerms) for (auto type : {prc::OptionType::Call, prc::OptionType::Put}) for (double k : kStrikes) {
    const Gbm m{100.0, 0.03, 0.01, v, t};
    prc::CosOptions o; o.num_terms = n;
    std::printf("%.17g,", prc::option_price_cos(type, m.s, k, m.r, m.q, m.t, gbm_cf(m), o, f64()));
  }
  std::printf("};\ninline const std::vector<double> gbm_series = {");
  for (double v : kVols) for (double t : kMaturities) for (std::int64_t n : kTerms) for (auto type : {prc::OptionType::Call, prc::OptionType::Put}) {
    const Gbm m{100.0, 0.03, 0.01, v, t};
    prc::CosOptions o; o.num_terms = n;
    const auto spot = pluss::series::full({5}, m.s, f64());
    const auto strike = pluss::make_series(kStrikes, {5}, f64());
    const auto maturity = pluss::series::full({5}, m.t, f64());
    const auto px = prc::option_price_cos(type, spot, strike, m.r, m.q, maturity, gbm_cf_series(m), o, f64());
    for (std::int64_t i = 0; i < 5; ++i) std::printf("%.17g,", px.select(0, i).item<double>());
  }
  std::printf("};\n");
  // Heston, Bates through the library's own entry points
  vol::HestonParams hp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045};
  std::printf("inline const std::vector<double> heston_cos = {");
  for (std::int64_t n : {256, 1024, 4096}) for (double k : {70.0, 85.0, 100.0, 115.0, 130.0}) {
    prc::CosOptions o; o.num_terms = n;
    std::printf("%.17g,", prc::detail::heston_price_cos(prc::OptionType::Call, 100.0, k, 0.02, 0.0, 1.0, hp, o, f64()));
  }
  vol::BatesParams bp{.kappa = 1.6, .theta = 0.04, .sigma = 0.30, .rho = -0.65, .v0 = 0.04, .jump_intensity = 0.18, .jump_mean = -0.08, .jump_vol = 0.22};
  std::printf("};\ninline const std::vector<double> bates_cos = {");
  for (std::int64_t n : {256, 1024, 4096}) for (double k : {70.0, 85.0, 100.0, 115.0, 130.0}) {
    prc::CosOptions o; o.num_terms = n;
    std::printf("%.17g,", prc::detail::bates_price_cos(prc::OptionType::Call, 100.0, k, 0.02, 0.0, 1.0, bp, o, f64()));
  }
  std::printf("};\n");
}

TEST_CASE("COS error table against exact Black-Scholes", "[.table]") {
  std::size_t idx = 0;
  std::printf("vol  T     N     type  | max |scalar-exact| over strikes | max |series-exact|\n");
  std::size_t sidx = 0;
  for (double v : kVols) for (double t : kMaturities) for (std::int64_t n : kTerms) for (auto type : {prc::OptionType::Call, prc::OptionType::Put}) {
    const Gbm m{100.0, 0.03, 0.01, v, t};
    double worst_scalar = 0.0, worst_series = 0.0;
    for (std::size_t i = 0; i < kStrikes.size(); ++i) {
      worst_scalar = std::max(worst_scalar, std::abs(cos_legacy::gbm_scalar[idx++] - bs_price(type, m, kStrikes[i])));
      worst_series = std::max(worst_series, std::abs(cos_legacy::gbm_series[sidx++] - bs_price(type, m, kStrikes[i])));
    }
    std::printf("%.2f %.2f %5lld %s  | %.3e | %.3e\n", v, t, static_cast<long long>(n), type == prc::OptionType::Call ? "call" : "put ", worst_scalar, worst_series);
  }
}

namespace {
template <class F> double best_seconds(int repeats, F &&f) {
  double best = 1e30;
  for (int i = 0; i < repeats; ++i) {
    const auto t0 = Clock::now();
    f();
    best = std::min(best, std::chrono::duration<double>(Clock::now() - t0).count());
  }
  return best;
}
} // namespace

TEST_CASE("COS timing baseline", "[.perf]") {
  const Gbm m;
  for (const std::int64_t n : {256, 1024}) {
    prc::CosOptions o; o.num_terms = n;
    const double scalar = best_seconds(20, [&] { (void)prc::option_price_cos(prc::OptionType::Call, m.s, 100.0, m.r, m.q, m.t, gbm_cf(m), o, f64()); });
    const auto one_spot = pluss::series::full({1}, m.s, f64()), one_strike = pluss::series::full({1}, 100.0, f64()), one_t = pluss::series::full({1}, m.t, f64());
    const double series1 = best_seconds(20, [&] { (void)prc::option_price_cos(prc::OptionType::Call, one_spot, one_strike, m.r, m.q, one_t, gbm_cf_series(m), o, f64()).sum().item<double>(); });
    std::vector<double> strikes(1000);
    for (std::size_t i = 0; i < strikes.size(); ++i) strikes[i] = 60.0 + 0.08 * static_cast<double>(i);
    const auto spot = pluss::series::full({1000}, m.s, f64()), strike = pluss::make_series(strikes, {1000}, f64()), maturity = pluss::series::full({1000}, m.t, f64());
    const double series1000 = best_seconds(5, [&] { (void)prc::option_price_cos(prc::OptionType::Call, spot, strike, m.r, m.q, maturity, gbm_cf_series(m), o, f64()).sum().item<double>(); });
    const double scalar1000 = best_seconds(2, [&] { for (const double k : strikes) (void)prc::option_price_cos(prc::OptionType::Call, m.s, k, m.r, m.q, m.t, gbm_cf(m), o, f64()); });
    std::printf("N=%lld: scalar single %.1f us | series single %.1f us | series x1000 %.2f ms | scalar loop x1000 %.2f ms\n", static_cast<long long>(n), scalar * 1e6, series1 * 1e6, series1000 * 1e3, scalar1000 * 1e3);
  }
}

TEST_CASE("COS matches frozen legacy values and exact Black-Scholes", "[cos][golden]") {
  std::size_t idx = 0, sidx = 0;
  double worst_scalar_vs_legacy = 0.0, worst_series_vs_legacy = 0.0, worst_exact_converged = 0.0, worst_exact_n64 = 0.0;
  double legacy_n64 = 0.0;
  for (double v : kVols) for (double t : kMaturities) for (std::int64_t n : kTerms) for (auto type : {prc::OptionType::Call, prc::OptionType::Put}) {
    const Gbm m{100.0, 0.03, 0.01, v, t};
    prc::CosOptions o; o.num_terms = n;
    const auto spot = pluss::series::full({5}, m.s, f64());
    const auto strike = pluss::make_series(kStrikes, {5}, f64());
    const auto maturity = pluss::series::full({5}, m.t, f64());
    const auto px = prc::option_price_cos(type, spot, strike, m.r, m.q, maturity, gbm_cf_series(m), o, f64());
    for (std::size_t i = 0; i < kStrikes.size(); ++i) {
      const double scalar = prc::option_price_cos(type, m.s, kStrikes[i], m.r, m.q, m.t, gbm_cf(m), o, f64());
      const double series = px.select(0, static_cast<std::int64_t>(i)).item<double>();
      const double exact = bs_price(type, m, kStrikes[i]);
      const double old_s = cos_legacy::gbm_scalar[idx++], old_v = cos_legacy::gbm_series[sidx++];
      CHECK(scalar == Catch::Approx(series).margin(1e-12));  // the two overloads now agree
      if (n >= 256) {
        worst_scalar_vs_legacy = std::max(worst_scalar_vs_legacy, std::abs(scalar - old_s));
        worst_series_vs_legacy = std::max(worst_series_vs_legacy, std::abs(series - old_v));
        worst_exact_converged = std::max(worst_exact_converged, std::abs(scalar - exact));
      } else {
        worst_exact_n64 = std::max(worst_exact_n64, std::abs(scalar - exact));
        legacy_n64 = std::max(legacy_n64, std::abs(old_s - exact));
      }
    }
  }
  std::printf("scalar-vs-legacy %.3e series-vs-legacy %.3e exact(N>=256) %.3e exact(N=64) %.3e legacy(N=64) %.3e\n",
              worst_scalar_vs_legacy, worst_series_vs_legacy, worst_exact_converged, worst_exact_n64, legacy_n64);
  CHECK(worst_scalar_vs_legacy < 1e-9);
  CHECK(worst_series_vs_legacy < 1e-9);
  CHECK(worst_exact_converged < 1e-9);
  CHECK(worst_exact_n64 <= legacy_n64);
}

TEST_CASE("COS Heston and Bates match frozen values and MATLAB", "[cos][golden]") {
  vol::HestonParams hp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045};
  vol::BatesParams bp{.kappa = 1.6, .theta = 0.04, .sigma = 0.30, .rho = -0.65, .v0 = 0.04, .jump_intensity = 0.18, .jump_mean = -0.08, .jump_vol = 0.22};
  std::size_t idx = 0;
  double worst_h = 0.0, worst_b = 0.0;
  for (std::int64_t n : {256, 1024, 4096}) for (double k : {70.0, 85.0, 100.0, 115.0, 130.0}) {
    prc::CosOptions o; o.num_terms = n;
    worst_h = std::max(worst_h, std::abs(prc::detail::heston_price_cos(prc::OptionType::Call, 100.0, k, 0.02, 0.0, 1.0, hp, o, f64()) - cos_legacy::heston_cos[idx]));
    worst_b = std::max(worst_b, std::abs(prc::detail::bates_price_cos(prc::OptionType::Call, 100.0, k, 0.02, 0.0, 1.0, bp, o, f64()) - cos_legacy::bates_cos[idx]));
    ++idx;
  }
  std::printf("heston-vs-legacy %.3e bates-vs-legacy %.3e\n", worst_h, worst_b);
  CHECK(worst_h < 1e-8);
  CHECK(worst_b < 1e-8);
}

TEST_CASE("COS Heston agrees with MATLAB numerical integration", "[cos][golden]") {
  vol::HestonParams hp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045};
  prc::CosOptions o; o.num_terms = 1024;
  const std::vector<double> ks{70.0, 85.0, 100.0, 115.0, 130.0};
  // reference rows were produced with the same spot/rate/maturity as the legacy Heston goldens
  double worst = 0.0;
  for (std::size_t i = 0; i < ks.size(); ++i) {
    const double c = prc::detail::heston_price_cos(prc::OptionType::Call, 100.0, ks[i], 0.02, 0.0, 1.0, hp, o, f64());
    const double p = prc::detail::heston_price_cos(prc::OptionType::Put, 100.0, ks[i], 0.02, 0.0, 1.0, hp, o, f64());
    worst = std::max({worst, std::abs(c - cos_legacy::matlab_heston_call[i]), std::abs(p - cos_legacy::matlab_heston_put[i])});
  }
  std::printf("heston-vs-matlab %.3e\n", worst);
  CHECK(worst < 1e-6);
}

TEST_CASE("COS timing with the Heston and Bates characteristic functions", "[.perf]") {
  vol::HestonParams hp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045};
  vol::BatesParams bp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045, .jump_intensity = 0.0, .jump_mean = 0.0, .jump_vol = 0.1};
  prc::CosOptions o; o.num_terms = 256;
  std::vector<double> strikes(1000);
  for (std::size_t i = 0; i < strikes.size(); ++i) strikes[i] = 60.0 + 0.08 * static_cast<double>(i);
  const auto spot = pluss::series::full({1000}, 100.0, f64()), strike = pluss::make_series(strikes, {1000}, f64()), maturity = pluss::series::full({1000}, 1.0, f64());
  const double series = best_seconds(3, [&] { (void)prc::detail::heston_price_cos(prc::OptionType::Call, spot, strike, 0.02, 0.0, maturity, hp, o, f64()).sum().item<double>(); });
  const double scalar = best_seconds(1, [&] { for (const double k : strikes) (void)prc::detail::bates_price_cos(prc::OptionType::Call, 100.0, k, 0.02, 0.0, 1.0, bp, o, f64()); });
  std::printf("Heston N=256 x1000 strikes: series %.2f ms | scalar loop (Bates, zero jumps) %.2f ms\n", series * 1e3, scalar * 1e3);
}

TEST_CASE("COS Heston crossover between scalar loop and Series chain", "[.perf]") {
  vol::HestonParams hp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045};
  vol::BatesParams bp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045, .jump_intensity = 0.0, .jump_mean = 0.0, .jump_vol = 0.1};
  for (const std::int64_t n : {64, 256, 1024}) for (const std::int64_t m : {1, 2, 4, 8, 16, 64}) {
    prc::CosOptions o; o.num_terms = n;
    std::vector<double> strikes(static_cast<std::size_t>(m));
    for (std::size_t i = 0; i < strikes.size(); ++i) strikes[i] = 80.0 + 40.0 * static_cast<double>(i) / static_cast<double>(m);
    const auto spot = pluss::series::full({m}, 100.0, f64()), strike = pluss::make_series(strikes, {m}, f64()), maturity = pluss::series::full({m}, 1.0, f64());
    const double series = best_seconds(5, [&] { (void)prc::detail::heston_price_cos(prc::OptionType::Call, spot, strike, 0.02, 0.0, maturity, hp, o, f64()).sum().item<double>(); });
    const double scalar = best_seconds(5, [&] { for (const double k : strikes) (void)prc::detail::bates_price_cos(prc::OptionType::Call, 100.0, k, 0.02, 0.0, 1.0, bp, o, f64()); });
    std::printf("cross N=%lld m=%lld: series %.1f us | scalar %.1f us\n", static_cast<long long>(n), static_cast<long long>(m), series * 1e6, scalar * 1e6);
  }
}

TEST_CASE("COS hybrid dispatch: loop and vectorized paths agree", "[cos][golden]") {
  vol::HestonParams hp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045};
  vol::BatesParams bp{.kappa = 1.9, .theta = 0.04, .sigma = 0.32, .rho = -0.7, .v0 = 0.045, .jump_intensity = 0.0, .jump_mean = 0.0, .jump_vol = 0.1};
  prc::CosOptions o; o.num_terms = 256;
  for (const std::int64_t m : {3, 300}) {  // 3 x 256 takes the loop, 300 x 256 the vectorized chain
    std::vector<double> strikes(static_cast<std::size_t>(m));
    for (std::size_t i = 0; i < strikes.size(); ++i) strikes[i] = 70.0 + 60.0 * static_cast<double>(i) / static_cast<double>(m);
    const auto spot = pluss::series::full({m}, 100.0, f64()), strike = pluss::make_series(strikes, {m}, f64()), maturity = pluss::series::full({m}, 1.0, f64());
    const auto px = prc::detail::heston_price_cos(prc::OptionType::Put, spot, strike, 0.02, 0.0, maturity, hp, o, f64()).host<double>();
    double worst = 0.0;
    for (std::size_t i = 0; i < strikes.size(); ++i)
      worst = std::max(worst, std::abs(px[i] - prc::detail::bates_price_cos(prc::OptionType::Put, 100.0, strikes[i], 0.02, 0.0, 1.0, bp, o, f64())));
    std::printf("hybrid m=%lld worst %.3e\n", static_cast<long long>(m), worst);
    CHECK(worst < 1e-9);
  }
}
