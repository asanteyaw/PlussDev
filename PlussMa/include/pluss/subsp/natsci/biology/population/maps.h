#pragma once

// Discrete-time density-dependent population maps x_{t+1} = f(x_t; params) and their dynamics, batched over lanes (parameter sweeps,
// initial conditions): iteration, attractors for bifurcation diagrams, Lyapunov exponents, cycle detection, periodic orbits and their
// multipliers, and superstable cycles (the Feigenbaum cascade). Every function is elementwise over Series of any shape.
//
// Maps (each with its analytic derivative d f / d x, checked against finite differences in the tests):
//   logistic    f = r x (1 - x)                              (May 1976 form, x in [0, 1])
//   ricker      f = x exp(r (1 - x / K))                     (Ricker 1954)
//   beverton_holt f = R0 x / (1 + (R0 - 1) x / K)            (Beverton & Holt 1957; monotone, never chaotic)
//   hassell     f = lambda x (1 + a x)^-b                    (Hassell 1975)
//   maynard_smith_slatkin f = lambda x / (1 + (a x)^b)       (Maynard Smith & Slatkin 1973)
//
// The Lyapunov exponent is the time average of ln |f'(x_t)| along an orbit after a burn-in; chaotic orbits are sensitive to rounding, so
// only statistics (exponent, period, attractor extent) are meaningful there.

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <functional>

namespace pluss::subsp::natsci::biology::population::maps {

using dynamics::Coeff;
using Map = std::function<Series(const Series &)>;

inline Series logistic(const Series &x, const Coeff &r) { return x.mul(r.on(x)).mul(x.neg().add(1.0)); }
inline Series logistic_derivative(const Series &x, const Coeff &r) { return x.mul(2.0).neg().add(1.0).mul(r.on(x)); }

inline Series ricker(const Series &x, const Coeff &r, const Coeff &k = 1.0) { return x.mul(x.div(k.on(x)).neg().add(1.0).mul(r.on(x)).exp()); }
inline Series ricker_derivative(const Series &x, const Coeff &r, const Coeff &k = 1.0) {
  const Series e = x.div(k.on(x)).neg().add(1.0).mul(r.on(x)).exp();
  return e.mul(x.div(k.on(x)).mul(r.on(x)).neg().add(1.0));
}

inline Series beverton_holt(const Series &x, const Coeff &r0, const Coeff &k = 1.0) {
  return x.mul(r0.on(x)).div(x.mul(r0.on(x).sub(1.0)).div(k.on(x)).add(1.0));
}
inline Series beverton_holt_derivative(const Series &x, const Coeff &r0, const Coeff &k = 1.0) {
  const Series d = x.mul(r0.on(x).sub(1.0)).div(k.on(x)).add(1.0);
  return r0.on(x).div(d.square());
}

inline Series hassell(const Series &x, const Coeff &lambda, const Coeff &a, const Coeff &b) { return x.mul(lambda.on(x)).mul(x.mul(a.on(x)).add(1.0).pow(b.on(x).neg())); }
inline Series hassell_derivative(const Series &x, const Coeff &lambda, const Coeff &a, const Coeff &b) {
  const Series u = x.mul(a.on(x)).add(1.0);
  return lambda.on(x).mul(u.pow(b.on(x).neg().sub(1.0))).mul(u.sub(x.mul(a.on(x)).mul(b.on(x))));
}

inline Series maynard_smith_slatkin(const Series &x, const Coeff &lambda, const Coeff &a, const Coeff &b) {
  return x.mul(lambda.on(x)).div(x.mul(a.on(x)).pow(b.on(x)).add(1.0));
}
inline Series maynard_smith_slatkin_derivative(const Series &x, const Coeff &lambda, const Coeff &a, const Coeff &b) {
  const Series p = x.mul(a.on(x)).pow(b.on(x));
  return lambda.on(x).mul(p.mul(b.on(x).neg().add(1.0)).add(1.0)).div(p.add(1.0).square());
}

// ---- iteration -------------------------------------------------------------------------------------------------------------
inline Series iterate(const Map &f, const Series &x0, std::int64_t steps) {
  dynamics::require(steps >= 0, "maps::iterate: steps must be non-negative");
  Series x = x0;
  for (std::int64_t i = 0; i < steps; ++i)
    x = f(x);
  return x;
}

// `keep` consecutive states after `burn` steps: [..., keep] (the attractor sample of a bifurcation diagram)
inline Series attractor(const Map &f, const Series &x0, std::int64_t burn, std::int64_t keep) {
  dynamics::require(keep > 0, "maps::attractor: keep must be positive");
  Series x = iterate(f, x0, burn);
  std::vector<Series> cols;
  for (std::int64_t i = 0; i < keep; ++i) {
    cols.push_back(x.unsqueeze(-1));
    x = f(x);
  }
  std::vector<Series> rest(cols.begin() + 1, cols.end());
  return rest.empty() ? cols[0] : cols[0].unite(rest, x0.dim());
}

// Lyapunov exponent (1/n) sum ln |f'(x_t)| over `steps` states after `burn` steps
inline Series lyapunov(const Map &f, const Map &df, const Series &x0, std::int64_t burn, std::int64_t steps) {
  dynamics::require(steps > 0, "maps::lyapunov: steps must be positive");
  Series x = iterate(f, x0, burn);
  Series total = x.mul(0.0);
  for (std::int64_t i = 0; i < steps; ++i) {
    total = total.add(df(x).abs().clamp(1e-300, 1e300).log());
    x = f(x);
  }
  return total.div(static_cast<double>(steps));
}

// Smallest period p <= max_period with |x_{t+p} - x_t| <= tol over the whole attractor sample [..., keep] (keep must exceed max_period);
// 0 where none is found (chaos or a period above max_period). Returns a Float64 Series of periods.
inline Series detect_period(const Series &orbit, std::int64_t max_period, double tol) {
  const std::int64_t keep = orbit.size(-1);
  dynamics::require(keep > max_period, "maps::detect_period: the sample must be longer than max_period");
  Series found = orbit.select(-1, 0).mul(0.0);
  for (std::int64_t p = max_period; p >= 1; --p) {
    const Series d = orbit.slice(-1, p, keep, 1).sub(orbit.slice(-1, 0, keep - p, 1)).abs().amax(-1, false);
    found = d.le(tol).where(found.mul(0.0).add(static_cast<double>(p)), found);
  }
  return found;
}

// Newton for a period-p point f^p(x) = x with the chain-rule multiplier; returns the point, converged lanes keep their value.
struct Cycle final {
  Series point = pluss::series::undefined();
  Series multiplier = pluss::series::undefined(); // prod f'(x_i) around the cycle
  bool converged{false};
};
inline Cycle periodic_point(const Map &f, const Map &df, const Series &guess, std::int64_t period, double tolerance = 1e-13, std::int64_t max_iterations = 100) {
  dynamics::require(period >= 1, "maps::periodic_point: period must be positive");
  Series x = guess;
  Cycle out;
  for (std::int64_t it = 0; it < max_iterations; ++it) {
    Series y = x, m = x.mul(0.0).add(1.0);
    for (std::int64_t i = 0; i < period; ++i) {
      m = m.mul(df(y));
      y = f(y);
    }
    const Series g = y.sub(x), dg = m.sub(1.0);
    const Series step = g.div(dg);
    x = x.sub(step);
    if (g.abs().max().item<double>() < tolerance) {
      out.converged = true;
      break;
    }
  }
  Series y = x, m = x.mul(0.0).add(1.0);
  for (std::int64_t i = 0; i < period; ++i) {
    m = m.mul(df(y));
    y = f(y);
  }
  out.point = x;
  out.multiplier = m;
  return out;
}

// Superstable cycle: parameter r with f_r^p(c) = c for the critical point c (the cycle then has multiplier 0). `map(x, r)` is the map,
// r_guess lanes are refined by a secant iteration on g(r) = f_r^p(c) - c; used to build the period-doubling cascade.
inline Series superstable_parameter(const std::function<Series(const Series &, const Series &)> &map, double critical_point, std::int64_t period,
                                    const Series &r_guess, double tolerance = 1e-14, std::int64_t max_iterations = 100) {
  const auto g = [&](const Series &r) {
    Series x = r.mul(0.0).add(critical_point);
    for (std::int64_t i = 0; i < period; ++i)
      x = map(x, r);
    return x.sub(critical_point);
  };
  Series r0 = r_guess, r1 = r_guess.add(1e-4);
  Series g0 = g(r0), g1 = g(r1);
  for (std::int64_t it = 0; it < max_iterations; ++it) {
    const Series denom = g1.sub(g0);
    const Series safe = denom.abs().lt(1e-300).where(denom.mul(0.0).add(1.0), denom);
    const Series r2 = r1.sub(g1.mul(r1.sub(r0)).div(safe));
    r0 = r1;
    g0 = g1;
    r1 = r2;
    g1 = g(r1);
    if (g1.abs().max().item<double>() < tolerance)
      break;
  }
  return r1;
}

} // namespace pluss::subsp::natsci::biology::population::maps
