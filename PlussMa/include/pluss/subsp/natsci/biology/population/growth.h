#pragma once

// Single-species growth laws. Every function is elementwise over Series of
// any shape; coefficients are Coeff (double or broadcastable Series).
// Rates are dn/dt; closed forms are provided where one exists so the
// numerical integrator can be validated against them.
//
// Sources: Wikipedia "Gompertz function" (X' = a ln(K/X) X,
// X(t) = K exp(ln(X0/K) e^{-a t})), "Allee effect" (strong cubic
// dN/dt = -rN(1-N/A)(1-N/K) and birth-death form), logistic closed form
// n(t) = K / (1 + ((K-n0)/n0) e^{-rt}).

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::population::growth {

using dynamics::Coeff;

inline Series exponential_rate(const Series &n, const Coeff &r) {
  return n.mul(r.on(n));
}

inline Series exponential(const Series &n0, const Coeff &r, const Coeff &t) {
  return n0.mul(r.on(n0).mul(t.on(n0)).exp());
}

inline Series logistic_rate(const Series &n, const Coeff &r, const Coeff &k) {
  dynamics::require_positive(k, n, "logistic: carrying capacity");
  return n.mul(r.on(n)).mul(n.div(k.on(n)).neg().add(1.0));
}

inline Series logistic(const Series &n0, const Coeff &r, const Coeff &k,
                       const Coeff &t) {
  dynamics::require_positive(n0, "logistic: n0");
  dynamics::require_positive(k, n0, "logistic: carrying capacity");
  const Series kk = k.on(n0);
  const Series decay = r.on(n0).mul(t.on(n0)).neg().exp();
  return kk.div(kk.sub(n0).div(n0).mul(decay).add(1.0));
}

inline Series theta_logistic_rate(const Series &n, const Coeff &r,
                                  const Coeff &k, const Coeff &theta) {
  dynamics::require_positive(k, n, "theta_logistic: carrying capacity");
  dynamics::require_positive(theta, n, "theta_logistic: theta");
  return n.mul(r.on(n))
      .mul(n.div(k.on(n)).pow(theta.on(n)).neg().add(1.0));
}

inline Series gompertz_rate(const Series &n, const Coeff &r, const Coeff &k) {
  dynamics::require_positive(n, "gompertz: n");
  dynamics::require_positive(k, n, "gompertz: carrying capacity");
  return n.mul(r.on(n)).mul(k.on(n).div(n).log());
}

inline Series gompertz(const Series &n0, const Coeff &r, const Coeff &k,
                       const Coeff &t) {
  dynamics::require_positive(n0, "gompertz: n0");
  dynamics::require_positive(k, n0, "gompertz: carrying capacity");
  const Series kk = k.on(n0);
  return kk.mul(n0.div(kk).log().mul(r.on(n0).mul(t.on(n0)).neg().exp()).exp());
}

// Strong Allee effect: dn/dt = r n (n/A - 1)(1 - n/K), 0 < A < K.
// Equilibria 0 (stable), A (unstable threshold), K (stable).
inline Series strong_allee_rate(const Series &n, const Coeff &r,
                                const Coeff &threshold, const Coeff &k) {
  dynamics::require_positive(threshold, n, "allee: threshold");
  dynamics::require_positive(k, n, "allee: carrying capacity");
  const Series a = threshold.on(n);
  const Series kk = k.on(n);
  dynamics::require(a.lt(kk).all().item<bool>(),
                    "allee: threshold must be below carrying capacity");
  return n.mul(r.on(n)).mul(n.div(a).sub(1.0)).mul(n.div(kk).neg().add(1.0));
}

// Birth-death Allee form: dn/dt = (b0 n/(b1 + n) - d - c n) n.
inline Series allee_birth_death_rate(const Series &n, const Coeff &b0,
                                     const Coeff &b1, const Coeff &d,
                                     const Coeff &c) {
  dynamics::require_nonnegative(b1, n, "allee: b1");
  const Series births = b0.on(n).mul(n).div(b1.on(n).add(n));
  return births.sub(d.on(n)).sub(c.on(n).mul(n)).mul(n);
}

} // namespace pluss::subsp::natsci::biology::population::growth
