// Bermudan and American options by the COS method (Fang and Oosterlee, "Pricing early-exercise and discrete barrier options by
// Fourier-cosine series expansions", Numer. Math. 114 (2009) 27-62): the option value is carried between exercise dates as cosine
// coefficients, the exercise point is found by root search on the continuation value, and the coefficients of the value are the
// payoff's on one side of it and the continuation value's on the other.
#include "pluss/subsp/math/finance/pricing.h"

#include "CodeM/core/Errors.h"
#include "cos_exercise_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <numbers>
#include <string>
#include <vector>

namespace pluss::subsp::math::finance::pricing {
using namespace detail;
namespace {

using Complex = std::complex<double>;
using Vec = std::vector<double>;

// Value of a put and its first and second derivatives in x = ln(S / K), read off the final cosine expansion.
struct Triple {
  double v = 0.0, vx = 0.0, vxx = 0.0;
  std::array<double, 4> dp{};  // derivatives of the value in (rate, carry, volatility, time to maturity), when requested
};

struct Bermudan {
  double a, b, rate, strike;
  std::size_t n;
  const std::vector<double>& step;  // lengths of the intervals between consecutive dates, from today
  double sigma, carry;

  Complex phi(double omega, double dt) const {  // characteristic function of the log-return over dt under the pricing measure
    const double mu = rate - carry - 0.5 * sigma * sigma;
    return std::exp(Complex(-0.5 * sigma * sigma * omega * omega * dt, omega * mu * dt));
  }

  // u_k = phi(k pi/(b-a)) V_k, with the first term halved: c(x) = e^{-r dt} Re sum_k u_k e^{i k pi (x-a)/(b-a)}
  std::vector<Complex> weights(const Vec& v, double dt) const {
    std::vector<Complex> u(n);
    for (std::size_t k = 0; k < n; ++k) u[k] = phi(static_cast<double>(k) * std::numbers::pi / (b - a), dt) * v[k] * (k == 0 ? 0.5 : 1.0);
    return u;
  }

  Triple price(double x0) const {
    const std::size_t dates = step.size();
    Vec v = internal::cos_payoff_coefficients(a, b, a, 0.0, strike, n, -1.0);  // value at the last date (a put)
    std::vector<Complex> scratch;
    Vec cont;
    for (std::size_t m = dates - 1; m >= 1; --m) {  // dates M-1 .. 1; today is date 0
      const double dt = step[m], disc = std::exp(-rate * dt);
      const std::vector<Complex> u = weights(v, dt);
      const double xs = internal::cos_exercise_point(a, b, strike, false, [&](double x, double& val, double& slope) { internal::cos_evaluate(a, b, u, disc, x, val, slope); });
      internal::cos_continuation_coefficients(a, b, xs, b, u, disc, scratch, cont);
      const Vec left = internal::cos_payoff_coefficients(a, b, a, xs, strike, n, -1.0);
      for (std::size_t k = 0; k < n; ++k) v[k] = left[k] + cont[k];
    }
    // the expansion is valid on all of [a, b]: the derivatives are read from it at neighbouring points, with no further recursion
    const auto u = weights(v, step[0]);
    const double disc = std::exp(-rate * step[0]), h = 1e-3 * (b - a) / static_cast<double>(n) * 8.0 + 1e-6;
    double value = 0.0, slope = 0.0, v_up = 0.0, s_up = 0.0, v_dn = 0.0, s_dn = 0.0;
    internal::cos_evaluate(a, b, u, disc, x0, value, slope);
    internal::cos_evaluate(a, b, u, disc, x0 + h, v_up, s_up);
    internal::cos_evaluate(a, b, u, disc, x0 - h, v_dn, s_dn);
    return Triple{value, slope, (s_up - s_dn) / (2.0 * h)};
  }

  // Derivatives of the value at x0 in (rate, carry, volatility, time to maturity) by differentiating the recursion itself. The truncation range and the exercise
  // points are held fixed: the value is continuous across an exercise point (payoff = continuation there), so it is stationary in it, and the range does not
  // belong to the problem. `dstep` holds the derivatives of the interval lengths in the time to maturity.
  std::array<double, 4> tangents(double x0, const Vec& dstep) const {
    const std::size_t dates = step.size();
    constexpr int P = 4;
    Vec v = internal::cos_payoff_coefficients(a, b, a, 0.0, strike, n, -1.0);
    std::array<Vec, P> dv;
    for (auto& d : dv) d.assign(n, 0.0);
    std::vector<Complex> scratch;
    Vec cont, dcont;
    const auto dlog_phi = [&](int p, double omega, double dt, double ddt) -> Complex {
      const double mu = rate - carry - 0.5 * sigma * sigma;
      switch (p) {
        case 0: return Complex(0.0, omega * dt);
        case 1: return Complex(0.0, -omega * dt);
        case 2: return Complex(-sigma * omega * omega * dt, -omega * sigma * dt);
        default: return Complex(-0.5 * sigma * sigma * omega * omega, omega * mu) * ddt;
      }
    };
    const auto weights_t = [&](const Vec& value, const std::array<Vec, P>& dvalue, double dt, double ddt, std::array<std::vector<Complex>, P>& du) {
      std::vector<Complex> u(n);
      for (int p = 0; p < P; ++p) du[static_cast<std::size_t>(p)].resize(n);
      for (std::size_t k = 0; k < n; ++k) {
        const double omega = static_cast<double>(k) * std::numbers::pi / (b - a), w = k == 0 ? 0.5 : 1.0;
        const Complex ph = phi(omega, dt);
        u[k] = ph * value[k] * w;
        for (int p = 0; p < P; ++p) du[static_cast<std::size_t>(p)][k] = ph * (dlog_phi(p, omega, dt, ddt) * value[k] + dvalue[static_cast<std::size_t>(p)][k]) * w;
      }
      return u;
    };
    const auto ddisc = [&](int p, double dt, double ddt, double disc) { return p == 0 ? -dt * disc : p == 3 ? -rate * disc * ddt : 0.0; };
    for (std::size_t m = dates - 1; m >= 1; --m) {
      const double dt = step[m], ddt = dstep[m], disc = std::exp(-rate * dt);
      std::array<std::vector<Complex>, P> du;
      const std::vector<Complex> u = weights_t(v, dv, dt, ddt, du);
      const double xs = internal::cos_exercise_point(a, b, strike, false, [&](double x, double& val, double& slope) { internal::cos_evaluate(a, b, u, disc, x, val, slope); });
      internal::cos_continuation_coefficients(a, b, xs, b, u, disc, scratch, cont);
      for (int p = 0; p < P; ++p) {
        internal::cos_continuation_coefficients(a, b, xs, b, du[static_cast<std::size_t>(p)], disc, scratch, dcont);
        const double dd = ddisc(p, dt, ddt, disc) / disc;
        for (std::size_t k = 0; k < n; ++k) dv[static_cast<std::size_t>(p)][k] = dcont[k] + dd * cont[k];  // the payoff on the exercise side has no parameter
      }
      const Vec left = internal::cos_payoff_coefficients(a, b, a, xs, strike, n, -1.0);
      for (std::size_t k = 0; k < n; ++k) v[k] = left[k] + cont[k];
    }
    std::array<std::vector<Complex>, P> du;
    const double dt = step[0], ddt = dstep[0], disc = std::exp(-rate * dt);
    const std::vector<Complex> u = weights_t(v, dv, dt, ddt, du);
    double value = 0.0, slope = 0.0;
    internal::cos_evaluate(a, b, u, disc, x0, value, slope);
    std::array<double, 4> out{};
    for (int p = 0; p < P; ++p) {
      double dval = 0.0, dslope = 0.0;
      internal::cos_evaluate(a, b, du[static_cast<std::size_t>(p)], disc, x0, dval, dslope);
      out[static_cast<std::size_t>(p)] = dval + ddisc(p, dt, ddt, disc) / disc * value;
    }
    return out;
  }
};

Triple bermudan_put(double spot, double strike, double rate, double carry, double sigma, const Vec& times, const CosOptions& o, const Vec* dtimes = nullptr) {
  Vec step(times.size());
  for (std::size_t i = 0; i < times.size(); ++i) step[i] = times[i] - (i == 0 ? 0.0 : times[i - 1]);
  const double horizon = times.back(), x0 = std::log(spot / strike);
  const double c1 = (rate - carry - 0.5 * sigma * sigma) * horizon, half = o.exercise_truncation * sigma * std::sqrt(horizon);
  double a = x0 + c1 - half, b = x0 + c1 + half;
  a = std::min(a, std::min(x0, 0.0) - 0.5);
  b = std::max(b, std::max(x0, 0.0) + 0.5);
  const Bermudan engine{a, b, rate, strike, static_cast<std::size_t>(o.exercise_terms), step, sigma, carry};
  Triple out = engine.price(x0);
  if (dtimes) {
    Vec dstep(times.size());
    for (std::size_t i = 0; i < times.size(); ++i) dstep[i] = (*dtimes)[i] - (i == 0 ? 0.0 : (*dtimes)[i - 1]);
    out.dp = engine.tangents(x0, dstep);
  }
  return out;
}

Triple european_put(double spot, double strike, double rate, double carry, double sigma, double maturity, const CosOptions& o, bool tangents = false) {
  const Vec d{1.0};
  return bermudan_put(spot, strike, rate, carry, sigma, Vec{maturity}, o, tangents ? &d : nullptr);
}

}  // namespace

double detail::black_scholes_price_cos(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                               double volatility, std::span<const double> dates, const CosOptions& o) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put, "subsp::math::finance::pricing::black_scholes_price_cos: option type must be Call or Put");
  PLUSS_CHECK(std::isfinite(spot) && spot > 0.0 && std::isfinite(strike) && strike > 0.0 && std::isfinite(volatility) && volatility > 0.0,
              "subsp::math::finance::pricing::black_scholes_price_cos: spot, strike and volatility must be finite and > 0");
  PLUSS_CHECK(o.exercise_terms >= 8 && o.extrapolation >= -1 && o.extrapolation <= 6,
              "subsp::math::finance::pricing::black_scholes_price_cos: exercise_terms >= 8 and extrapolation in [-1, 6] are required");
  if (!(maturity > 0.0)) return type == OptionType::Call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0);
  // McDonald-Schroder symmetry reduces a call to a put
  const bool call = type == OptionType::Call;
  const double s = call ? strike : spot, k = call ? spot : strike, r = call ? dividend_yield : rate, q = call ? rate : dividend_yield;
  if (style == ExerciseStyle::European) return european_put(s, k, r, q, volatility, maturity, o).v;
  if (style == ExerciseStyle::Bermudan) {
    Vec times;
    for (const double t : dates) {
      PLUSS_CHECK(std::isfinite(t) && t > 0.0, "subsp::math::finance::pricing::black_scholes_price_cos: exercise dates must be finite and > 0");
      if (t < maturity) times.push_back(t);
    }
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end(), [](double x, double y) { return std::abs(x - y) < 1e-12; }), times.end());
    times.push_back(maturity);
    return bermudan_put(s, k, r, q, volatility, times, o).v;
  }
  // American: v_AM(d) = (64 v(2^(d+3)) - 56 v(2^(d+2)) + 14 v(2^(d+1)) - v(2^d)) / 21
  auto equally_spaced = [&](std::int64_t count) {
    Vec times(static_cast<std::size_t>(count));
    for (std::int64_t i = 0; i < count; ++i) times[static_cast<std::size_t>(i)] = maturity * static_cast<double>(i + 1) / static_cast<double>(count);
    return bermudan_put(s, k, r, q, volatility, times, o).v;
  };
  const std::int64_t base = std::int64_t{1} << (o.extrapolation < 0 ? 3 : o.extrapolation);
  return (64.0 * equally_spaced(8 * base) - 56.0 * equally_spaced(4 * base) + 14.0 * equally_spaced(2 * base) - equally_spaced(base)) / 21.0;
}

ScalarGreeks detail::black_scholes_cos_greeks(OptionType type, ExerciseStyle style, double spot, double strike, double rate, double dividend_yield, double maturity,
                                              double volatility, std::span<const double> dates, const CosOptions& o) {
  PLUSS_CHECK(type == OptionType::Call || type == OptionType::Put, "subsp::math::finance::pricing::black_scholes_price_cos: option type must be Call or Put");
  ScalarGreeks g;
  if (!(maturity > 0.0)) {
    g.price = type == OptionType::Call ? std::max(spot - strike, 0.0) : std::max(strike - spot, 0.0);
    g.delta = type == OptionType::Call ? (spot > strike ? 1.0 : 0.0) : (spot < strike ? -1.0 : 0.0);
    return g;
  }
  const bool call = type == OptionType::Call;
  // the put problem (McDonald-Schroder symmetry for a call)
  const double s = call ? strike : spot, k = call ? spot : strike, r = call ? dividend_yield : rate, q = call ? rate : dividend_yield;
  // value and x-derivatives of the put for a given parameter set: the Bermudan sum, or the Richardson combination of four Bermudan prices for an American option
  const auto put_triple = [&](double sp, double kp, double rp, double qp, double sigma, double T) -> Triple {
    if (style == ExerciseStyle::European) return european_put(sp, kp, rp, qp, sigma, T, o, true);
    if (style == ExerciseStyle::Bermudan) {
      Vec times;
      for (const double t : dates) if (std::isfinite(t) && t > 0.0 && t < T) times.push_back(t);
      std::sort(times.begin(), times.end());
      times.erase(std::unique(times.begin(), times.end(), [](double x, double y) { return std::abs(x - y) < 1e-12; }), times.end());
      times.push_back(T);
      Vec dtimes(times.size(), 0.0);
      dtimes.back() = 1.0;   // the last date is the maturity
      return bermudan_put(sp, kp, rp, qp, sigma, times, o, &dtimes);
    }
    auto equally_spaced = [&](std::int64_t count) {
      Vec times(static_cast<std::size_t>(count)), dtimes(static_cast<std::size_t>(count));
      for (std::int64_t i = 0; i < count; ++i) {
        times[static_cast<std::size_t>(i)] = T * static_cast<double>(i + 1) / static_cast<double>(count);
        dtimes[static_cast<std::size_t>(i)] = static_cast<double>(i + 1) / static_cast<double>(count);
      }
      return bermudan_put(sp, kp, rp, qp, sigma, times, o, &dtimes);
    };
    const std::int64_t base = std::int64_t{1} << (o.extrapolation < 0 ? 3 : o.extrapolation);
    const Triple a = equally_spaced(8 * base), b = equally_spaced(4 * base), c = equally_spaced(2 * base), d = equally_spaced(base);
    const auto mix = [&](double x8, double x4, double x2, double x1) { return (64.0 * x8 - 56.0 * x4 + 14.0 * x2 - x1) / 21.0; };
    Triple out{mix(a.v, b.v, c.v, d.v), mix(a.vx, b.vx, c.vx, d.vx), mix(a.vxx, b.vxx, c.vxx, d.vxx)};
    for (std::size_t p = 0; p < 4; ++p) out.dp[p] = mix(a.dp[p], b.dp[p], c.dp[p], d.dp[p]);
    return out;
  };
  const Triple base = put_triple(s, k, r, q, volatility, maturity);
  const double put_delta = base.vx / s, put_gamma = (base.vxx - base.vx) / (s * s);   // derivatives in the put's spot
  const double d_rate = base.dp[0], d_carry = base.dp[1], d_sigma = base.dp[2], d_tau = base.dp[3];
  g.price = base.v;
  g.vega = d_sigma;
  g.theta = -d_tau;
  if (!call) {
    g.delta = put_delta; g.gamma = put_gamma; g.rho = d_rate;
  } else {
    // C(S, K, r, q) = P'(S' = K, K' = S, r' = q, q' = r); P' is homogeneous of degree one in (S', K')
    g.delta = (base.v - s * put_delta) / k;      // here the put's spot s is the call's strike and its strike k is the call's spot
    g.gamma = s * s * put_gamma / (k * k);
    g.rho = d_carry;
  }
  return g;
}

}  // namespace pluss::subsp::math::finance::pricing
