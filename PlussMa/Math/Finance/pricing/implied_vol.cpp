#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kInvSqrt2 = 0.70710678118654752440;
constexpr double kInvSqrt2Pi = 0.39894228040143267794;

struct BsValueVega {
  double price = std::numeric_limits<double>::quiet_NaN();
  double vega = std::numeric_limits<double>::quiet_NaN();
};

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_vector_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::math::finance::pricing::") + op + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::math::finance::pricing::") + op + ": expected non-scalar Series");
  if (sizes.size() == 1) return x.reshape({sizes[0]});
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::math::finance::pricing::") + op + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

void validate_type_(OptionType type, const char* op) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put,
              std::string("subsp::math::finance::pricing::") + op + ": option type must be Call or Put (OptionType::Auto is only valid for an instrument that supplies is_call)");
}

void validate_options_(const ImpliedVolOptions& options) {
  PLUSS_CHECK(std::isfinite(options.lower_vol) && options.lower_vol > 0.0,
              "subsp::math::finance::pricing::implied_vol: lower_vol must be finite and > 0");
  PLUSS_CHECK(std::isfinite(options.upper_vol) && options.upper_vol > options.lower_vol,
              "subsp::math::finance::pricing::implied_vol: upper_vol must be finite and > lower_vol");
  PLUSS_CHECK(std::isfinite(options.tolerance) && options.tolerance > 0.0,
              "subsp::math::finance::pricing::implied_vol: tolerance must be finite and > 0");
  PLUSS_CHECK(options.max_iterations > 0,
              "subsp::math::finance::pricing::implied_vol: max_iterations must be > 0");
  PLUSS_CHECK(std::isfinite(options.vega_floor) && options.vega_floor > 0.0,
              "subsp::math::finance::pricing::implied_vol: vega_floor must be finite and > 0");
}

double norm_cdf_(double x) {
  return 0.5 * std::erfc(-x * kInvSqrt2);
}

double norm_pdf_(double x) {
  return kInvSqrt2Pi * std::exp(-0.5 * x * x);
}

double discounted_intrinsic_(OptionType type, double spot, double strike, double rate, double dividend_yield, double ttm) {
  const double s_disc = spot * std::exp(-dividend_yield * ttm);
  const double k_disc = strike * std::exp(-rate * ttm);
  if (type == OptionType::Call) return std::max(0.0, s_disc - k_disc);
  return std::max(0.0, k_disc - s_disc);
}

BsValueVega bs_price_vega_(OptionType type,
                           double spot,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double ttm,
                           double vol) {
  const double sqrt_t = std::sqrt(ttm);
  const double vol_sqrt_t = vol * sqrt_t;
  const double d1 = (std::log(spot / strike) + (rate - dividend_yield + 0.5 * vol * vol) * ttm) / vol_sqrt_t;
  const double d2 = d1 - vol_sqrt_t;
  const double s_disc = spot * std::exp(-dividend_yield * ttm);
  const double k_disc = strike * std::exp(-rate * ttm);
  const double cdf_d1 = norm_cdf_(d1);
  const double cdf_d2 = norm_cdf_(d2);

  double price = std::numeric_limits<double>::quiet_NaN();
  if (type == OptionType::Call) {
    price = s_disc * cdf_d1 - k_disc * cdf_d2;
  } else {
    price = k_disc * norm_cdf_(-d2) - s_disc * norm_cdf_(-d1);
  }
  const double vega = s_disc * sqrt_t * norm_pdf_(d1);
  return BsValueVega{.price = price, .vega = vega};
}

double objective_(OptionType type,
                  double sigma,
                  double target,
                  double spot,
                  double strike,
                  double rate,
                  double dividend_yield,
                  double ttm) {
  return bs_price_vega_(type, spot, strike, rate, dividend_yield, ttm, sigma).price - target;
}

double solve_bisection_(OptionType type,
                        double target,
                        double spot,
                        double strike,
                        double rate,
                        double dividend_yield,
                        double ttm,
                        double lo,
                        double hi,
                        double flo,
                        double price_tol,
                        const ImpliedVolOptions& options) {
  double a = lo;
  double b = hi;
  double fa = flo;
  double fb = objective_(type, b, target, spot, strike, rate, dividend_yield, ttm);
  for (std::int64_t i = 0; i < options.max_iterations; ++i) {
    const double m = 0.5 * (a + b);
    const double fm = objective_(type, m, target, spot, strike, rate, dividend_yield, ttm);
    if (!std::isfinite(fm)) return std::numeric_limits<double>::quiet_NaN();
    if (std::abs(fm) <= price_tol || 0.5 * std::abs(b - a) <= options.tolerance) return m;
    if ((fa <= 0.0 && fm >= 0.0) || (fa >= 0.0 && fm <= 0.0)) {
      b = m;
      fb = fm;
    } else {
      a = m;
      fa = fm;
    }
  }
  (void)fb;
  // Not converged within max_iterations: report that no solution was found rather than the last midpoint.
  const double m = 0.5 * (a + b);
  const double fm = objective_(type, m, target, spot, strike, rate, dividend_yield, ttm);
  if (std::isfinite(fm) && (std::abs(fm) <= price_tol || 0.5 * std::abs(b - a) <= options.tolerance)) return m;
  return std::numeric_limits<double>::quiet_NaN();
}

double solve_secant_(OptionType type,
                     double target,
                     double spot,
                     double strike,
                     double rate,
                     double dividend_yield,
                     double ttm,
                     double lo,
                     double hi,
                     double flo,
                     double price_tol,
                     const ImpliedVolOptions& options) {
  double a = lo;
  double b = hi;
  double fa = flo;
  double fb = objective_(type, b, target, spot, strike, rate, dividend_yield, ttm);
  double x0 = a;
  double f0 = fa;
  double x1 = b;
  double f1 = fb;

  for (std::int64_t i = 0; i < options.max_iterations; ++i) {
    if (std::abs(f1) <= price_tol || std::abs(b - a) <= options.tolerance) return x1;
    double x2 = std::numeric_limits<double>::quiet_NaN();
    const double denom = f1 - f0;
    if (std::isfinite(denom) && std::abs(denom) > 1e-16) {
      x2 = x1 - f1 * (x1 - x0) / denom;
    }
    if (!std::isfinite(x2) || x2 <= a || x2 >= b) x2 = 0.5 * (a + b);
    const double f2 = objective_(type, x2, target, spot, strike, rate, dividend_yield, ttm);
    if (!std::isfinite(f2)) return std::numeric_limits<double>::quiet_NaN();

    if ((fa <= 0.0 && f2 >= 0.0) || (fa >= 0.0 && f2 <= 0.0)) {
      b = x2;
      fb = f2;
    } else {
      a = x2;
      fa = f2;
    }
    x0 = x1;
    f0 = f1;
    x1 = x2;
    f1 = f2;
  }
  (void)fb;
  return x1;
}

double solve_newton_(OptionType type,
                     double target,
                     double spot,
                     double strike,
                     double rate,
                     double dividend_yield,
                     double ttm,
                     double lo,
                     double hi,
                     double flo,
                     double price_tol,
                     const ImpliedVolOptions& options) {
  double a = lo;
  double b = hi;
  double fa = flo;
  double x = 0.5 * (a + b);

  for (std::int64_t i = 0; i < options.max_iterations; ++i) {
    const BsValueVega pv = bs_price_vega_(type, spot, strike, rate, dividend_yield, ttm, x);
    const double fx = pv.price - target;
    if (!std::isfinite(fx) || !std::isfinite(pv.vega)) return std::numeric_limits<double>::quiet_NaN();
    if (std::abs(fx) <= price_tol) return x;

    if ((fa <= 0.0 && fx >= 0.0) || (fa >= 0.0 && fx <= 0.0)) {
      b = x;
    } else {
      a = x;
      fa = fx;
    }

    double xn = std::numeric_limits<double>::quiet_NaN();
    if (std::abs(pv.vega) > options.vega_floor) xn = x - fx / pv.vega;
    if (!std::isfinite(xn) || xn <= a || xn >= b) xn = 0.5 * (a + b);
    if (std::abs(xn - x) <= options.tolerance) return xn;
    x = xn;
  }
  return x;
}

double solve_brent_(OptionType type,
                    double target,
                    double spot,
                    double strike,
                    double rate,
                    double dividend_yield,
                    double ttm,
                    double lo,
                    double hi,
                    double flo,
                    double price_tol,
                    const ImpliedVolOptions& options) {
  double a = lo;
  double b = hi;
  double fa = flo;
  double fb = objective_(type, b, target, spot, strike, rate, dividend_yield, ttm);
  if (!std::isfinite(fb)) return std::numeric_limits<double>::quiet_NaN();
  if ((fa < 0.0 && fb < 0.0) || (fa > 0.0 && fb > 0.0)) return std::numeric_limits<double>::quiet_NaN();

  double c = a;
  double fc = fa;
  double d = b - a;
  double e = d;

  for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
    if ((fb < 0.0 && fc < 0.0) || (fb > 0.0 && fc > 0.0)) {
      c = a;
      fc = fa;
      d = b - a;
      e = d;
    }
    if (std::abs(fc) < std::abs(fb)) {
      // (a, b, c) <- (b, c, b): the new contrapoint c is the old b, as in Brent's algorithm
      const double old_b = b;
      const double old_fb = fb;
      a = old_b;
      fa = old_fb;
      b = c;
      fb = fc;
      c = old_b;
      fc = old_fb;
    }

    const double tol1 = 2.0 * std::numeric_limits<double>::epsilon() * std::abs(b) + 0.5 * options.tolerance;
    const double m = 0.5 * (c - b);
    if (std::abs(m) <= tol1 || std::abs(fb) <= price_tol) return b;

    if (std::abs(e) >= tol1 && std::abs(fa) > std::abs(fb)) {
      double p = 0.0;
      double q = 0.0;
      const double s = fb / fa;
      if (a == c) {
        p = 2.0 * m * s;
        q = 1.0 - s;
      } else {
        const double q1 = fa / fc;
        const double r = fb / fc;
        p = s * (2.0 * m * q1 * (q1 - r) - (b - a) * (r - 1.0));
        q = (q1 - 1.0) * (r - 1.0) * (s - 1.0);
      }
      if (p > 0.0) q = -q;
      p = std::abs(p);
      const double min1 = 3.0 * m * q - std::abs(tol1 * q);
      const double min2 = std::abs(e * q);
      if (2.0 * p < std::min(min1, min2)) {
        e = d;
        d = p / q;
      } else {
        d = m;
        e = m;
      }
    } else {
      d = m;
      e = m;
    }

    a = b;
    fa = fb;
    if (std::abs(d) > tol1) {
      b += d;
    } else {
      b += (m > 0.0 ? tol1 : -tol1);
    }
    fb = objective_(type, b, target, spot, strike, rate, dividend_yield, ttm);
    if (!std::isfinite(fb)) return std::numeric_limits<double>::quiet_NaN();
  }
  return b;
}

double implied_vol_scalar_(OptionType type,
                           double option_price,
                           double spot,
                           double strike,
                           double rate,
                           double dividend_yield,
                           double maturity,
                           const ImpliedVolOptions& options) {
  validate_type_(type, "implied_vol");
  validate_options_(options);
  if (!std::isfinite(option_price) || !std::isfinite(spot) || !std::isfinite(strike) || !std::isfinite(rate) ||
      !std::isfinite(dividend_yield) || !std::isfinite(maturity) || option_price < 0.0 || spot <= 0.0 || strike <= 0.0 ||
      maturity <= 0.0) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  const double intrinsic = discounted_intrinsic_(type, spot, strike, rate, dividend_yield, maturity);
  // The price tolerance is relative to the time value (price less intrinsic value; the whole price out of the money), which is the part
  // of the price that carries the volatility. A price of 1e-15 is then matched to within 1e-23 and the volatility it implies is
  // resolved, instead of every volatility below some absolute threshold being accepted, and a deep in-the-money price is not judged
  // against its large intrinsic value. The floor is a few units in the last place of the price, which is all the price can resolve.
  const double time_value = std::max(option_price - intrinsic, 0.0);
  const double price_tol = std::max(options.tolerance * std::max(time_value, std::numeric_limits<double>::min()),
                                    64.0 * std::numeric_limits<double>::epsilon() * option_price);
  const double lo = options.lower_vol;
  const double hi = options.upper_vol;
  const double plo = bs_price_vega_(type, spot, strike, rate, dividend_yield, maturity, lo).price;
  const double phi = bs_price_vega_(type, spot, strike, rate, dividend_yield, maturity, hi).price;
  if (!std::isfinite(plo) || !std::isfinite(phi)) return std::numeric_limits<double>::quiet_NaN();
  if (option_price < intrinsic - price_tol) return std::numeric_limits<double>::quiet_NaN();
  if (option_price > phi + price_tol) return std::numeric_limits<double>::quiet_NaN();

  const double flo = plo - option_price;
  if (std::abs(flo) <= price_tol) return lo;
  const double fhi = phi - option_price;
  if (std::abs(fhi) <= price_tol) return hi;
  if ((flo < 0.0 && fhi < 0.0) || (flo > 0.0 && fhi > 0.0)) return std::numeric_limits<double>::quiet_NaN();

  double sigma = std::numeric_limits<double>::quiet_NaN();
  switch (options.method) {
    case ImpliedVolMethod::Auto:
    case ImpliedVolMethod::Brent:
      sigma = solve_brent_(type, option_price, spot, strike, rate, dividend_yield, maturity, lo, hi, flo, price_tol, options);
      break;
    case ImpliedVolMethod::Newton:
      sigma = solve_newton_(type, option_price, spot, strike, rate, dividend_yield, maturity, lo, hi, flo, price_tol, options);
      break;
    case ImpliedVolMethod::Bisection:
      sigma = solve_bisection_(type, option_price, spot, strike, rate, dividend_yield, maturity, lo, hi, flo, price_tol, options);
      break;
    case ImpliedVolMethod::Secant:
      sigma = solve_secant_(type, option_price, spot, strike, rate, dividend_yield, maturity, lo, hi, flo, price_tol, options);
      break;
  }
  const double residual = objective_(type, sigma, option_price, spot, strike, rate, dividend_yield, maturity);
  if (!std::isfinite(sigma) || sigma < lo || sigma > hi || !std::isfinite(residual) ||
      std::abs(residual) > std::max(price_tol * 10.0, 1e-300)) {
    sigma = solve_bisection_(type, option_price, spot, strike, rate, dividend_yield, maturity, lo, hi, flo, price_tol, options);
  }
  return sigma;
}

Series implied_vol_series_(OptionType type,
                           const Series& option_price,
                           const Series& spot,
                           const Series& time_to_maturity,
                           double strike,
                           double rate,
                           double dividend_yield,
                           const ImpliedVolOptions& options,
                           Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series p = as_vector_(option_price, "implied_vol").to(out_cfg.platform, out_cfg.dtype);
  const Series s = as_vector_(spot, "implied_vol").to(out_cfg.platform, out_cfg.dtype);
  const Series tau = as_vector_(time_to_maturity, "implied_vol").to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(p.numel() == s.numel() && s.numel() == tau.numel(),
              "subsp::math::finance::pricing::implied_vol: option_price/spot/time_to_maturity size mismatch");

  // Solve on host buffers and build the result once; rebuilding the output array element by element would be quadratic in its length.
  const std::int64_t n = p.numel();
  const std::vector<double> prices = p.to(pluss::pCPU, ScalarType::Float64).host<double>();
  const std::vector<double> spots = s.to(pluss::pCPU, ScalarType::Float64).host<double>();
  const std::vector<double> maturities = tau.to(pluss::pCPU, ScalarType::Float64).host<double>();
  std::vector<double> result(static_cast<std::size_t>(n), std::numeric_limits<double>::quiet_NaN());
  for (std::int64_t i = 0; i < n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    result[k] = implied_vol_scalar_(type, prices[k], spots[k], strike, rate, dividend_yield, maturities[k], options);
  }
  return pluss::make_series(result, {n}, out_cfg);
}

} // namespace

double implied_vol(OptionType type,
                   double option_price,
                   double spot,
                   double strike,
                   double rate,
                   double dividend_yield,
                   double maturity,
                   const ImpliedVolOptions& options) {
  return implied_vol_scalar_(type, option_price, spot, strike, rate, dividend_yield, maturity, options);
}

Series implied_vol(OptionType type,
                   const Series& option_price,
                   const Series& spot,
                   double strike,
                   double rate,
                   double dividend_yield,
                   double maturity,
                   const ImpliedVolOptions& options,
                   Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series s = as_vector_(spot, "implied_vol").to(out_cfg.platform, out_cfg.dtype);
  const Series tau = pluss::series::full({s.numel()}, maturity, out_cfg);
  return implied_vol_series_(type, option_price, s, tau, strike, rate, dividend_yield, options, out_cfg);
}

Series implied_vol(OptionType type,
                   const Series& option_price,
                   const Series& spot,
                   const Series& time_to_maturity,
                   double strike,
                   double rate,
                   double dividend_yield,
                   const ImpliedVolOptions& options,
                   Config cfg) {
  return implied_vol_series_(type, option_price, spot, time_to_maturity, strike, rate, dividend_yield, options, cfg);
}

} // namespace pluss::subsp::math::finance::pricing
