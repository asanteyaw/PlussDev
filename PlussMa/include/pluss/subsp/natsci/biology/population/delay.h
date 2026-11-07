#pragma once

// Delay-differential population models: a fixed-step method-of-steps integrator (classical RK4 with cubic-Hermite interpolation of the
// stored history at half steps) for dx/dt = f(x(t), x(t - tau)), batched over lanes, plus the classical delayed growth laws and the exact
// linear stability theory of  x' = -a x(t) - b x(t - tau)  through the Lambert W function.
//
//   Hutchinson (delayed logistic)  N' = r N (1 - N(t - tau) / K)               (Hutchinson 1948); Hopf bifurcation at r tau = pi / 2 with
//                                                                             angular frequency r, period 4 tau at onset
//   Nicholson's blowflies          N' = -d N + P N(t - tau) exp(-N(t - tau) / N0)  (Gurney, Blythe & Nisbet 1980 form); N* = N0 ln(P / d)
//
// Linear theory: the characteristic equation lambda + a + b e^{-lambda tau} = 0 has roots lambda_k = -a + W_k(-b tau e^{a tau}) / tau;
// the rightmost root is the principal branch W_0 (Hayes 1950; Shinozaki & Mori 2006), so the equilibrium is stable iff Re W_0(.) < a tau.
// `lambert_w0_complex` solves w e^w = z for complex z (real/imaginary parts as separate Series) by Newton continuation along a path from 0.
// The original papers were not obtained; every statement is verified against mpmath's complex Lambert W and against the integrator.

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <numbers>
#include <functional>

namespace pluss::subsp::natsci::biology::population::delay {

using dynamics::Coeff;

// ---- growth laws ------------------------------------------------------------------------------------------------------------
inline Series hutchinson_rate(const Series &n, const Series &n_delayed, const Coeff &r, const Coeff &k) {
  return n.mul(r.on(n)).mul(n_delayed.div(k.on(n)).neg().add(1.0));
}
inline Series nicholson_rate(const Series &n, const Series &n_delayed, const Coeff &death, const Coeff &production, const Coeff &scale) {
  return n.mul(death.on(n)).neg().add(n_delayed.mul(production.on(n)).mul(n_delayed.div(scale.on(n)).neg().exp()));
}
inline double hutchinson_critical_delay(double r) { return std::numbers::pi_v<double> / (2.0 * r); }
inline double nicholson_equilibrium(double death, double production, double scale) { return scale * std::log(production / death); }

// ---- complex Lambert W (principal branch) ------------------------------------------------------------------------------------------
struct Complex final {
  Series re = pluss::series::undefined(), im = pluss::series::undefined();
};
inline Complex lambert_w0_complex(const Series &z_re, const Series &z_im, int path_steps = 64, int newton_per_step = 5) {
  // Homotopy W0(z_t) along z_t = t z + i H sin(pi t) from W0(0) = 0 with Newton warm starts; the small arc above the real axis puts real
  // z < -1/e on the upper side of the cut (the principal branch convention Im W0 >= 0 there) and avoids passing through the branch point.
  const Series height = z_re.square().add(z_im.square()).sqrt().mul(0.3).clamp(0.05, 1e300);
  Series wr = z_re.mul(0.0), wi = z_re.mul(0.0);
  for (int j = 1; j <= path_steps; ++j) {
    const double t = static_cast<double>(j) / path_steps;
    const Series tr = z_re.mul(t), ti = z_im.mul(t).add(height.mul(std::sin(std::numbers::pi_v<double> * t)));
    for (int i = 0; i < newton_per_step; ++i) {
      const Series e = wr.exp(), c = wi.cos(), s = wi.sin();
      const Series er = e.mul(c), ei = e.mul(s);                                                       // e^w
      const Series fr = wr.mul(er).sub(wi.mul(ei)).sub(tr), fi = wr.mul(ei).add(wi.mul(er)).sub(ti);  // w e^w - z
      const Series dr = er.mul(wr.add(1.0)).sub(ei.mul(wi)), di = er.mul(wi).add(ei.mul(wr.add(1.0))); // e^w (w + 1)
      const Series den = dr.square().add(di.square()).clamp(1e-300, 1e300);
      wr = wr.sub(fr.mul(dr).add(fi.mul(di)).div(den));
      wi = wi.sub(fi.mul(dr).sub(fr.mul(di)).div(den));
    }
  }
  // the last point of the path has z_t = z (sin(pi) = 0 up to rounding): a final polish removes the O(1e-16 H) offset
  for (int i = 0; i < 3; ++i) {
    const Series e = wr.exp(), c = wi.cos(), s = wi.sin();
    const Series er = e.mul(c), ei = e.mul(s);
    const Series fr = wr.mul(er).sub(wi.mul(ei)).sub(z_re), fi = wr.mul(ei).add(wi.mul(er)).sub(z_im);
    const Series dr = er.mul(wr.add(1.0)).sub(ei.mul(wi)), di = er.mul(wi).add(ei.mul(wr.add(1.0)));
    const Series den = dr.square().add(di.square()).clamp(1e-300, 1e300);
    wr = wr.sub(fr.mul(dr).add(fi.mul(di)).div(den));
    wi = wi.sub(fi.mul(dr).sub(fr.mul(di)).div(den));
  }
  return {wr, wi};
}

struct Root final {
  Series re = pluss::series::undefined();
  Series im = pluss::series::undefined();
};
// rightmost root of lambda + a + b e^{-lambda tau} = 0 (real a, b, tau: roots come in conjugate pairs, the one with Im >= 0 is returned)
inline Root rightmost_root(const Series &a, const Series &b, const Series &tau) {
  const Series z = b.mul(tau).neg().mul(a.mul(tau).exp());
  const Complex w = lambert_w0_complex(z, z.mul(0.0));
  return {a.neg().add(w.re.div(tau)), w.im.abs().div(tau)};
}

// ---- method-of-steps integrator ---------------------------------------------------------------------------------------------
using DelayRate = std::function<Series(const Series &x, const Series &x_delayed)>;
using History = std::function<Series(double t)>; // state [..., S] for t <= 0

struct Path final {
  std::vector<double> times;      // t_0 = 0 ... t_N
  std::vector<Series> states;     // N + 1 states, each [..., S]
};
// `steps_per_delay` RK4 steps per delay tau (tau is therefore an exact multiple of the step); integrates to t_end >= 0.
inline Path integrate(const DelayRate &f, const History &history, double tau, double t_end, std::int64_t steps_per_delay) {
  dynamics::require(tau > 0.0 && t_end >= 0.0 && steps_per_delay >= 2, "delay::integrate: need tau > 0, t_end >= 0 and at least two steps per delay");
  const double dt = tau / static_cast<double>(steps_per_delay);
  const auto total = static_cast<std::int64_t>(std::ceil(t_end / dt - 1e-12));
  const std::int64_t m = steps_per_delay;
  Path path;
  path.times.push_back(0.0);
  path.states.push_back(history(0.0));
  std::vector<Series> rates; // f at stored nodes
  const auto delayed_at_node = [&](std::int64_t k) { return k >= 0 ? path.states[static_cast<std::size_t>(k)] : history(static_cast<double>(k) * dt); }; // x(t_k) for any integer k
  for (std::int64_t k = 0; k < total; ++k) {
    const Series &x = path.states[static_cast<std::size_t>(k)];
    const Series xd0 = delayed_at_node(k - m), xd1 = delayed_at_node(k - m + 1);
    const Series k1 = f(x, xd0);
    if (rates.size() <= static_cast<std::size_t>(k))
      rates.push_back(k1);
    // x(t_k + dt/2 - tau): between nodes k-m and k-m+1
    Series xdh = pluss::series::undefined();
    if (k - m >= 0) {
      const Series f0 = rates[static_cast<std::size_t>(k - m)];
      const Series f1 = (static_cast<std::size_t>(k - m + 1) < rates.size()) ? rates[static_cast<std::size_t>(k - m + 1)] : f(path.states[static_cast<std::size_t>(k - m + 1)], delayed_at_node(k + 1 - 2 * m));
      xdh = xd0.add(xd1).mul(0.5).add(f0.sub(f1).mul(dt / 8.0));
    } else {
      xdh = history((static_cast<double>(k) + 0.5) * dt - tau);
    }
    const Series k2 = f(x.add(k1.mul(0.5 * dt)), xdh);
    const Series k3 = f(x.add(k2.mul(0.5 * dt)), xdh);
    const Series k4 = f(x.add(k3.mul(dt)), xd1);
    path.states.push_back(x.add(k1.add(k2.mul(2.0)).add(k3.mul(2.0)).add(k4).mul(dt / 6.0)));
    path.times.push_back(static_cast<double>(k + 1) * dt);
  }
  return path;
}

} // namespace pluss::subsp::natsci::biology::population::delay
