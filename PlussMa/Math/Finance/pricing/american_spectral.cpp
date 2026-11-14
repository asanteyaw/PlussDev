// American options by the integral-equation spectral collocation of Andersen, Lake and Offengenden ("High performance American option
// pricing", Journal of Computational Finance, 2016): Chebyshev interpolation of the exercise boundary in the square root of time, a
// fixed-point equation for it, and Gauss-Legendre quadrature of the early-exercise premium.
#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
using namespace detail;
namespace {

constexpr double kInvSqrt2Pi = 0.39894228040143267794;

// Forward-mode number carrying the derivatives with respect to (rate, carry, volatility): the sensitivities of the solved boundary and of the price are
// obtained exactly by running the same formulas on it.
struct Dual {
  double v = 0.0;
  double g[3] = {0.0, 0.0, 0.0};
  Dual() = default;
  Dual(double value) : v(value) {}
};
Dual operator+(const Dual& a, const Dual& b) { Dual r(a.v + b.v); for (int i = 0; i < 3; ++i) r.g[i] = a.g[i] + b.g[i]; return r; }
Dual operator-(const Dual& a, const Dual& b) { Dual r(a.v - b.v); for (int i = 0; i < 3; ++i) r.g[i] = a.g[i] - b.g[i]; return r; }
Dual operator-(const Dual& a) { Dual r(-a.v); for (int i = 0; i < 3; ++i) r.g[i] = -a.g[i]; return r; }
Dual operator*(const Dual& a, const Dual& b) { Dual r(a.v * b.v); for (int i = 0; i < 3; ++i) r.g[i] = a.g[i] * b.v + a.v * b.g[i]; return r; }
Dual operator/(const Dual& a, const Dual& b) {
  Dual r(a.v / b.v);
  for (int i = 0; i < 3; ++i) r.g[i] = (a.g[i] - r.v * b.g[i]) / b.v;
  return r;
}
Dual& operator+=(Dual& a, const Dual& b) { a = a + b; return a; }
Dual apply(const Dual& a, double value, double slope) { Dual r(value); for (int i = 0; i < 3; ++i) r.g[i] = slope * a.g[i]; return r; }
Dual exp(const Dual& a) { const double e = std::exp(a.v); return apply(a, e, e); }
Dual log(const Dual& a) { return apply(a, std::log(a.v), 1.0 / a.v); }
Dual sqrt(const Dual& a) { const double r = std::sqrt(a.v); return apply(a, r, 0.5 / r); }
Dual erfc(const Dual& a) { return apply(a, std::erfc(a.v), -1.1283791670955126 * std::exp(-a.v * a.v)); }
using std::exp;
using std::log;
using std::sqrt;
using std::erfc;

template <class T> T pdf(T x) { return kInvSqrt2Pi * exp(-0.5 * x * x); }
template <class T> T cdf(T x) { return 0.5 * erfc(-x * 0.70710678118654752440); }

// Gauss-Legendre nodes and weights on [-1, 1] by Newton iteration on the Legendre recurrence.
void gauss_legendre(std::int64_t n, std::vector<double>& x, std::vector<double>& w) {
  x.assign(static_cast<std::size_t>(n), 0.0);
  w.assign(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < (n + 1) / 2; ++i) {
    double z = std::cos(std::numbers::pi * (static_cast<double>(i) + 0.75) / (static_cast<double>(n) + 0.5));
    double dp = 1.0;
    for (int it = 0; it < 100; ++it) {
      double p0 = 1.0, p1 = z;
      for (std::int64_t k = 2; k <= n; ++k) {
        const double p2 = ((2.0 * static_cast<double>(k) - 1.0) * z * p1 - (static_cast<double>(k) - 1.0) * p0) / static_cast<double>(k);
        p0 = p1;
        p1 = p2;
      }
      dp = static_cast<double>(n) * (z * p1 - p0) / (z * z - 1.0);
      const double dz = p1 / dp;
      z -= dz;
      if (std::abs(dz) < 1e-16) break;
    }
    x[static_cast<std::size_t>(i)] = -z;
    x[static_cast<std::size_t>(n - 1 - i)] = z;
    w[static_cast<std::size_t>(i)] = w[static_cast<std::size_t>(n - 1 - i)] = 2.0 / ((1.0 - z * z) * dp * dp);
  }
}

template <class T> T d_plus(T t, T z, T r, T q, T sigma) { return (log(z) + (r - q + 0.5 * sigma * sigma) * t) / (sigma * sqrt(t)); }
template <class T> T d_minus(T t, T z, T r, T q, T sigma) { return (log(z) + (r - q - 0.5 * sigma * sigma) * t) / (sigma * sqrt(t)); }

template <class T> T european_put(T s, T k, T r, T q, T t, T sigma) {
  const T dm = d_minus<T>(t, s / k, r, q, sigma), dp = d_plus<T>(t, s / k, r, q, sigma);
  return k * exp(-r * t) * cdf<T>(-dm) - s * exp(-q * t) * cdf<T>(-dp);
}

// The put boundary of one problem, held as a Chebyshev interpolant of ln(X/B)^2 in z = 2 sqrt(tau / T) - 1.
struct PutBoundary {
  double strike, rate, carry, sigma, maturity;  // carry = dividend yield
  double asymptote;                             // X = B(0), the boundary at expiry
  std::vector<double> node_x, node_tau, node_q, bary;

  double value_q(double z) const {
    const auto n = static_cast<std::int64_t>(node_x.size()) - 1;
    double num = 0.0, den = 0.0;
    for (std::int64_t i = 0; i <= n; ++i) {
      const auto k = static_cast<std::size_t>(i);
      const double d = z - node_x[k];
      if (d == 0.0) return node_q[k];
      const double t = bary[k] / d;
      num += t * node_q[k];
      den += t;
    }
    return num / den;
  }

  double at(double tau) const {
    if (tau <= 0.0) return asymptote;
    const double z = std::clamp(2.0 * std::sqrt(tau / maturity) - 1.0, -1.0, 1.0);
    return asymptote * std::exp(-std::sqrt(std::max(value_q(z), 0.0)));
  }
};

PutBoundary solve_boundary(double strike, double rate, double carry, double maturity, double sigma, const SpectralOptions& o) {
  PLUSS_CHECK(o.nodes >= 3 && o.quadrature >= 4 && o.max_iterations >= 1,
              "subsp::math::finance::pricing::spectral_price: nodes >= 3, quadrature >= 4 and max_iterations >= 1 are required");
  PutBoundary b;
  b.strike = strike;
  b.rate = rate;
  b.carry = carry;
  b.sigma = sigma;
  b.maturity = maturity;
  b.asymptote = carry > 0.0 ? strike * std::min(1.0, rate / carry) : strike;
  const auto n = o.nodes;
  b.node_x.resize(static_cast<std::size_t>(n) + 1);
  b.node_tau.resize(b.node_x.size());
  b.bary.resize(b.node_x.size());
  b.node_q.assign(b.node_x.size(), 0.0);
  for (std::int64_t i = 0; i <= n; ++i) {
    const auto k = static_cast<std::size_t>(i);
    b.node_x[k] = std::cos(std::numbers::pi * static_cast<double>(i) / static_cast<double>(n));
    b.node_tau[k] = maturity * 0.25 * (1.0 + b.node_x[k]) * (1.0 + b.node_x[k]);
    b.bary[k] = ((i % 2 == 0) ? 1.0 : -1.0) * ((i == 0 || i == n) ? 0.5 : 1.0);
    b.node_q[k] = 0.7225 * sigma * sigma * b.node_tau[k];  // leading behaviour B ~ X (1 - 0.85 sigma sqrt(tau))
  }
  b.node_q[static_cast<std::size_t>(n)] = 0.0;  // tau = 0: B = X
  std::vector<double> gx, gw;
  gauss_legendre(o.quadrature, gx, gw);
  // u = tau sin^2(theta): the integrands are smooth in sqrt(u) and sqrt(tau - u), so they are analytic in theta on [0, pi/2]
  const auto l = static_cast<std::size_t>(o.quadrature);
  std::vector<double> sin2(l), cos2(l), wt(l);
  for (std::size_t k = 0; k < l; ++k) {
    const double theta = 0.25 * std::numbers::pi * (gx[k] + 1.0);
    const double s = std::sin(theta), c = std::cos(theta);
    sin2[k] = s * s;
    cos2[k] = c * c;
    wt[k] = gw[k] * 0.25 * std::numbers::pi * 2.0 * s * c;
  }

  std::vector<double> next(b.node_q.size());
  bool converged = false;
  for (std::int64_t sweep = 0; sweep < o.max_iterations; ++sweep) {
    double change = 0.0;
    for (std::int64_t i = 0; i < n; ++i) {
      const auto k = static_cast<std::size_t>(i);
      const double tau = b.node_tau[k];
      const double bi = b.asymptote * std::exp(-std::sqrt(std::max(b.node_q[k], 0.0)));
      // s = tau sin^2(theta) is the time elapsed back from tau, tau - s = tau cos^2(theta) the earlier time at which B is read
      double int_n = 0.0, int_d = 0.0;
      for (std::size_t j = 0; j < l; ++j) {
        const double elapsed = tau * sin2[j], early = tau * cos2[j];
        const double ratio = bi / b.at(early);
        const double dm = d_minus(elapsed, ratio, rate, carry, sigma), dp = d_plus(elapsed, ratio, rate, carry, sigma);
        const double inv = 1.0 / (sigma * std::sqrt(elapsed));
        int_n += wt[j] * tau * std::exp(rate * early) * pdf(dm) * inv;
        int_d += wt[j] * tau * std::exp(carry * early) * (cdf(dp) + pdf(dp) * inv);
      }
      const double st = sigma * std::sqrt(tau);
      const double dm = d_minus(tau, bi / strike, rate, carry, sigma), dp = d_plus(tau, bi / strike, rate, carry, sigma);
      const double num = pdf(dm) / st + rate * int_n;
      const double den = pdf(dp) / st + cdf(dp) + carry * int_d;
      const double fresh = std::min(strike * std::exp(-(rate - carry) * tau) * num / den, b.asymptote);
      next[k] = std::pow(std::log(b.asymptote / fresh), 2);
      change = std::max(change, std::abs(fresh - bi) / bi);
    }
    for (std::int64_t i = 0; i < n; ++i) b.node_q[static_cast<std::size_t>(i)] = next[static_cast<std::size_t>(i)];
    if (change < o.tolerance) {
      converged = true;
      break;
    }
  }
  PLUSS_CHECK(converged, "subsp::math::finance::pricing::spectral_price: the exercise boundary did not converge; raise max_iterations or nodes");
  return b;
}

double put_price(double spot, double strike, double rate, double carry, double maturity, double sigma, const SpectralOptions& o) {
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0 && std::isfinite(strike) && strike > 0.0 && std::isfinite(sigma) && sigma > 0.0,
              "subsp::math::finance::pricing::spectral_price: spot, strike and volatility must be finite and > 0");
  if (!(maturity > 0.0)) return std::max(strike - spot, 0.0);
  const double european = european_put(spot, strike, rate, carry, maturity, sigma);
  if (!(rate > 0.0)) return european;  // with non-positive interest a put is never exercised early
  const auto b = solve_boundary(strike, rate, carry, maturity, sigma, o);
  if (spot <= b.at(maturity)) return strike - spot;

  std::vector<double> gx, gw;
  gauss_legendre(o.quadrature, gx, gw);
  double premium = 0.0;
  for (std::size_t j = 0; j < gx.size(); ++j) {
    const double theta = 0.25 * std::numbers::pi * (gx[j] + 1.0);
    const double s = std::sin(theta), c = std::cos(theta);
    const double u = maturity * s * s;
    const double ratio = spot / b.at(maturity * c * c);
    const double f = rate * strike * std::exp(-rate * u) * cdf(-d_minus(u, ratio, rate, carry, sigma)) -
                     carry * spot * std::exp(-carry * u) * cdf(-d_plus(u, ratio, rate, carry, sigma));
    premium += gw[j] * 0.25 * std::numbers::pi * 2.0 * maturity * s * c * f;
  }
  return std::max(european + premium, strike - spot);
}

// Put price at (spot, tau) with the boundary of an already solved problem held fixed. The boundary is stationary in the time to maturity, so a shorter
// maturity reads the same interpolant, and the boundary does not depend on the spot.
double put_with_boundary(const PutBoundary& b, double spot, double tau, const SpectralOptions& o, double rate, double carry, double sigma) {
  const double european = european_put(spot, b.strike, rate, carry, tau, sigma);
  if (spot <= b.at(tau)) return b.strike - spot;
  std::vector<double> gx, gw;
  gauss_legendre(o.quadrature, gx, gw);
  double premium = 0.0;
  for (std::size_t j = 0; j < gx.size(); ++j) {
    const double theta = 0.25 * std::numbers::pi * (gx[j] + 1.0);
    const double s = std::sin(theta), c = std::cos(theta);
    const double u = tau * s * s;
    const double ratio = spot / b.at(tau * c * c);
    const double f = rate * b.strike * std::exp(-rate * u) * cdf(-d_minus(u, ratio, rate, carry, sigma)) -
                     carry * spot * std::exp(-carry * u) * cdf(-d_plus(u, ratio, rate, carry, sigma));
    premium += gw[j] * 0.25 * std::numbers::pi * 2.0 * tau * s * c * f;
  }
  return std::max(european + premium, b.strike - spot);
}

double put_with_boundary(const PutBoundary& b, double spot, double tau, const SpectralOptions& o) {
  return put_with_boundary(b, spot, tau, o, b.rate, b.carry, b.sigma);
}

// Exact derivatives of the put price in (rate, carry, volatility). The boundary solves the fixed point q = G(q, theta), so its sensitivity solves
// dq = G_q dq + G_theta: iterate that linear fixed point by running the sweep on dual numbers around the solved boundary, then carry the boundary
// sensitivity through the pricing formula. No further boundary solve and no bumped price is involved.
Dual price_tangent(const PutBoundary& b, double spot, double tau, const SpectralOptions& o) {
  Dual rate(b.rate), carry(b.carry), sigma(b.sigma);
  rate.g[0] = 1.0;
  carry.g[1] = 1.0;
  sigma.g[2] = 1.0;
  const double strike = b.strike;
  Dual asym(strike);
  if (b.carry > 0.0 && b.rate < b.carry) asym = Dual(strike) * rate / carry;
  const auto n = static_cast<std::size_t>(o.nodes);

  std::vector<double> gx, gw;
  gauss_legendre(o.quadrature, gx, gw);
  const auto l = static_cast<std::size_t>(o.quadrature);
  std::vector<double> sin2(l), cos2(l), wt(l);
  for (std::size_t k = 0; k < l; ++k) {
    const double theta = 0.25 * std::numbers::pi * (gx[k] + 1.0);
    const double s = std::sin(theta), c = std::cos(theta);
    sin2[k] = s * s;
    cos2[k] = c * c;
    wt[k] = gw[k] * 0.25 * std::numbers::pi * 2.0 * s * c;
  }

  std::vector<Dual> q(b.node_q.size());
  for (std::size_t k = 0; k < q.size(); ++k) q[k].v = b.node_q[k];
  const auto boundary_at = [&](const std::vector<Dual>& nodes, double time) -> Dual {
    if (time <= 0.0) return asym;
    const double z = std::clamp(2.0 * std::sqrt(time / b.maturity) - 1.0, -1.0, 1.0);
    Dual num, den, hit;
    bool exact = false;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
      const double d = z - b.node_x[i];
      if (d == 0.0) { hit = nodes[i]; exact = true; break; }
      const double t = b.bary[i] / d;
      num = num + nodes[i] * Dual(t);
      den = den + Dual(t);
    }
    const Dual value = exact ? hit : num / den;
    return asym * exp(-sqrt(value.v > 0.0 ? value : Dual(0.0)));
  };

  bool converged = false;
  for (std::int64_t sweep = 0; sweep < 4 * o.max_iterations && !converged; ++sweep) {
    std::vector<Dual> next(q.size());
    double change = 0.0;
    for (std::size_t k = 0; k < n; ++k) {
      const double time = b.node_tau[k];
      const Dual bi = asym * exp(-sqrt(q[k].v > 0.0 ? q[k] : Dual(0.0)));
      Dual int_n, int_d;
      for (std::size_t j = 0; j < l; ++j) {
        const double elapsed = time * sin2[j], early = time * cos2[j];
        const Dual ratio = bi / boundary_at(q, early);
        const Dual dm = d_minus<Dual>(Dual(elapsed), ratio, rate, carry, sigma), dp = d_plus<Dual>(Dual(elapsed), ratio, rate, carry, sigma);
        const Dual inv = Dual(1.0) / (sigma * Dual(std::sqrt(elapsed)));
        int_n += Dual(wt[j] * time) * exp(rate * Dual(early)) * pdf<Dual>(dm) * inv;
        int_d += Dual(wt[j] * time) * exp(carry * Dual(early)) * (cdf<Dual>(dp) + pdf<Dual>(dp) * inv);
      }
      const Dual st = sigma * Dual(std::sqrt(time));
      const Dual dm = d_minus<Dual>(Dual(time), bi / Dual(strike), rate, carry, sigma), dp = d_plus<Dual>(Dual(time), bi / Dual(strike), rate, carry, sigma);
      const Dual num = pdf<Dual>(dm) / st + rate * int_n;
      const Dual den = pdf<Dual>(dp) / st + cdf<Dual>(dp) + carry * int_d;
      Dual fresh = Dual(strike) * exp(-(rate - carry) * Dual(time)) * num / den;
      if (fresh.v > asym.v) fresh = asym;
      const Dual ln = log(asym / fresh);
      next[k] = ln * ln;
      next[k].v = b.node_q[k];  // the value stays on the solved boundary; only its sensitivity is iterated
      for (int i = 0; i < 3; ++i) change = std::max(change, std::abs(next[k].g[i] - q[k].g[i]) / (1.0 + std::abs(next[k].g[i])));
    }
    for (std::size_t k = 0; k < n; ++k) q[k] = next[k];
    converged = change < 1e-13;
  }
  PLUSS_CHECK(converged, "subsp::math::finance::pricing::spectral_price: the boundary sensitivities did not converge; raise max_iterations or nodes");

  const Dual spot_d(spot), tau_d(tau), strike_d(strike);
  Dual premium;
  for (std::size_t j = 0; j < l; ++j) {
    const double u = tau * sin2[j];
    const Dual ratio = spot_d / boundary_at(q, tau * cos2[j]);
    const Dual f = rate * strike_d * exp(-rate * Dual(u)) * cdf<Dual>(-d_minus<Dual>(Dual(u), ratio, rate, carry, sigma)) -
                   carry * spot_d * exp(-carry * Dual(u)) * cdf<Dual>(-d_plus<Dual>(Dual(u), ratio, rate, carry, sigma));
    premium += Dual(wt[j] * tau) * f;
  }
  return european_put<Dual>(spot_d, strike_d, rate, carry, tau_d, sigma) + premium;
}

struct PutGreeks {
  double price = 0.0, d_spot = 0.0, d_spot2 = 0.0, d_tau = 0.0, d_rate = 0.0, d_carry = 0.0, d_sigma = 0.0;
};

// Put price and its derivatives. The spot derivatives and the derivative in the time to maturity use the solved boundary (it is independent of the spot and
// stationary in the time to maturity), so they cost no further solve; the derivatives in the rate, the carry and the volatility differentiate the boundary equation exactly.
PutGreeks put_greeks(double spot, double strike, double rate, double carry, double maturity, double sigma, const SpectralOptions& o) {
  PutGreeks g;
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0 && std::isfinite(strike) && strike > 0.0 && std::isfinite(sigma) && sigma > 0.0,
              "subsp::math::finance::pricing::spectral_price: spot, strike and volatility must be finite and > 0");
  if (!(maturity > 0.0)) {
    g.price = std::max(strike - spot, 0.0);
    g.d_spot = spot < strike ? -1.0 : 0.0;
    return g;
  }
  const bool early = rate > 0.0;
  PutBoundary b;
  if (early) b = solve_boundary(strike, rate, carry, maturity, sigma, o);
  const auto value = [&](double s, double tau) {
    if (!early) return european_put(s, strike, rate, carry, tau, sigma);
    return put_with_boundary(b, s, tau, o);
  };
  g.price = value(spot, maturity);
  if (early && spot <= b.at(maturity)) {   // in the exercise region the value is the payoff
    g.d_spot = -1.0;
    return g;
  }
  const double hs = 1e-3 * spot;
  const double up = value(spot + hs, maturity), down = value(spot - hs, maturity);
  g.d_spot = (up - down) / (2.0 * hs);
  g.d_spot2 = (up - 2.0 * g.price + down) / (hs * hs);
  const double ht = std::min(1e-3 * maturity, 0.25 * maturity);
  g.d_tau = (3.0 * g.price - 4.0 * value(spot, maturity - ht) + value(spot, maturity - 2.0 * ht)) / (2.0 * ht);   // backward, second order, on the same boundary
  // Derivatives in the rate, the carry and the volatility by forward-mode differentiation of the pricing formula and of the boundary equation.
  Dual t;
  if (early) {
    t = price_tangent(b, spot, maturity, o);
  } else {
    Dual r(rate), q(carry), v(sigma);
    r.g[0] = 1.0;
    q.g[1] = 1.0;
    v.g[2] = 1.0;
    t = european_put<Dual>(Dual(spot), Dual(strike), r, q, Dual(maturity), v);
  }
  g.d_rate = t.g[0];
  g.d_carry = t.g[1];
  g.d_sigma = t.g[2];
  return g;
}

}  // namespace

double detail::spectral_price(OptionType type, double spot, double strike, double rate, double dividend_yield, double maturity, double volatility,
                      const SpectralOptions& options) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put, "subsp::math::finance::pricing::spectral_price: option type must be Call or Put");
  // McDonald-Schroder symmetry: C(S, K, r, q) = P(K, S, q, r)
  if (type == OptionType::Call) return put_price(strike, spot, dividend_yield, rate, maturity, volatility, options);
  return put_price(spot, strike, rate, dividend_yield, maturity, volatility, options);
}

pluss::codem::Series detail::spectral_boundary(OptionType type, double strike, double rate, double dividend_yield, double maturity, double volatility,
                                       const pluss::codem::Series& times, const SpectralOptions& options) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put, "subsp::math::finance::pricing::spectral_boundary: option type must be Call or Put");
  PLUSS_CHECK(maturity > 0.0 && volatility > 0.0 && strike > 0.0, "subsp::math::finance::pricing::spectral_boundary: maturity, volatility and strike must be > 0");
  const auto t = times.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({times.numel()}).host<double>();
  const bool call = type == OptionType::Call;
  const double r = call ? dividend_yield : rate, q = call ? rate : dividend_yield;
  std::vector<double> out(t.size());
  if (!(r > 0.0)) {
    std::fill(out.begin(), out.end(), call ? std::numeric_limits<double>::infinity() : 0.0);  // never exercised early
  } else {
    const auto b = solve_boundary(strike, r, q, maturity, volatility, options);
    for (std::size_t i = 0; i < t.size(); ++i) {
      PLUSS_CHECK(t[i] >= 0.0 && t[i] <= maturity, "subsp::math::finance::pricing::spectral_boundary: times must lie in [0, maturity]");
      out[i] = b.at(t[i]);
    }
    if (call) for (auto& v : out) v = strike * strike / v;  // the call boundary is K^2 over the swapped put's
  }
  return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
}

ScalarGreeks detail::spectral_greeks(OptionType type, double spot, double strike, double rate, double dividend_yield, double maturity, double volatility,
                                     const SpectralOptions& options) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put, "subsp::math::finance::pricing::spectral_price: option type must be Call or Put");
  ScalarGreeks out;
  if (type == OptionType::Put) {
    const PutGreeks g = put_greeks(spot, strike, rate, dividend_yield, maturity, volatility, options);
    out.price = g.price; out.delta = g.d_spot; out.gamma = g.d_spot2; out.vega = g.d_sigma; out.theta = -g.d_tau; out.rho = g.d_rate;
    return out;
  }
  // McDonald-Schroder: C(S, K, r, q) = P'(S' = K, K' = S, r' = q, q' = r). The put is homogeneous of degree one in (S', K'), so the derivative in its strike
  // K' = S follows from its derivative in S': dC/dS = (C - K dP'/dS') / S and d2C/dS2 = K^2 d2P'/dS'^2 / S^2; the rate of the call is the carry of the put.
  const PutGreeks g = put_greeks(strike, spot, dividend_yield, rate, maturity, volatility, options);
  out.price = g.price;
  out.delta = (g.price - strike * g.d_spot) / spot;
  out.gamma = strike * strike * g.d_spot2 / (spot * spot);
  out.vega = g.d_sigma;
  out.theta = -g.d_tau;
  out.rho = g.d_carry;
  return out;
}

}  // namespace pluss::subsp::math::finance::pricing
