#pragma once

// Multi-species interaction models. State [..., n] (species last axis),
// interaction matrices [n, n] or [..., n, n]; all lanes evaluated in single
// Series ops.
//
// Sources: Wikipedia "Lotka-Volterra equations" (first integral
// V = delta x - gamma ln x + beta y - alpha ln y, equilibrium
// (gamma/delta, alpha/beta)), "Competitive Lotka-Volterra equations",
// "Functional response" (Holling II: aR/(1+ahR); III: aR^k/(1+ahR^k)).

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::population::interaction {

using dynamics::Coeff;

// Generalised LV: dn_i = n_i (r_i + sum_j A_ij n_j).
inline Series glv_rate(const Series &n, const Series &r, const Series &a) {
  dynamics::require_square(a, "glv: interaction matrix");
  dynamics::require_species(n, a.size(-1), "glv: state");
  return n.mul(r.add(dynamics::matvec(a, n)));
}

// Interior equilibrium n* = -A^{-1} r (batched). Check feasibility with
// is_feasible; stability needs the Jacobian diag(n*) A.
inline Series glv_equilibrium(const Series &r, const Series &a) {
  return dynamics::solve(a, r.neg());
}

inline Series is_feasible(const Series &equilibrium) {
  return equilibrium.gt(0.0).all(-1, false);
}

// Community (Jacobian) matrix at the interior equilibrium: diag(n*) A.
inline Series glv_jacobian_at(const Series &n_star, const Series &a) {
  return n_star.unsqueeze(-1).mul(a);
}

// Competitive LV: dn_i = r_i n_i (1 - sum_j alpha_ij n_j / K_i).
inline Series competitive_rate(const Series &n, const Series &r,
                               const Series &k, const Series &alpha) {
  dynamics::require_square(alpha, "competitive: alpha");
  dynamics::require_positive(k, "competitive: K");
  return n.mul(r).mul(dynamics::matvec(alpha, n).div(k).neg().add(1.0));
}

// Holling functional responses (prey density R).
inline Series holling1(const Series &prey, const Coeff &attack) {
  return prey.mul(attack.on(prey));
}
inline Series holling2(const Series &prey, const Coeff &attack,
                       const Coeff &handling) {
  const Series ar = prey.mul(attack.on(prey));
  return ar.div(ar.mul(handling.on(prey)).add(1.0));
}
inline Series holling3(const Series &prey, const Coeff &attack,
                       const Coeff &handling, const Coeff &exponent = 2.0) {
  const Series ar = prey.pow(exponent.on(prey)).mul(attack.on(prey));
  return ar.div(ar.mul(handling.on(prey)).add(1.0));
}

// Classical predator-prey on state [..., 2] = (prey x, predator y):
//   dx = alpha x - beta x y,   dy = delta x y - gamma y.
inline Series predator_prey_rate(const Series &s, const Coeff &alpha,
                                 const Coeff &beta, const Coeff &gamma,
                                 const Coeff &delta) {
  dynamics::require_species(s, 2, "predator_prey: state");
  const Series x = s.select(-1, 0);
  const Series y = s.select(-1, 1);
  const Series dx = x.mul(alpha.on(x).sub(beta.on(x).mul(y)));
  const Series dy = y.mul(delta.on(x).mul(x).sub(gamma.on(x)));
  return dx.unsqueeze(-1).unite(std::vector<Series>{dy.unsqueeze(-1)}, -1);
}

inline Series predator_prey_equilibrium(const Coeff &alpha, const Coeff &beta,
                                        const Coeff &gamma,
                                        const Coeff &delta,
                                        const Series &like) {
  const Series x = gamma.on(like).div(delta.on(like));
  const Series y = alpha.on(like).div(beta.on(like));
  return x.unsqueeze(-1).unite(std::vector<Series>{y.unsqueeze(-1)}, -1);
}

// Conserved quantity of the classical system.
inline Series predator_prey_invariant(const Series &s, const Coeff &alpha,
                                      const Coeff &beta, const Coeff &gamma,
                                      const Coeff &delta) {
  dynamics::require_species(s, 2, "predator_prey: state");
  const Series x = s.select(-1, 0);
  const Series y = s.select(-1, 1);
  return delta.on(x).mul(x).sub(gamma.on(x).mul(x.log()))
      .add(beta.on(x).mul(y)).sub(alpha.on(x).mul(y.log()));
}

// Rosenzweig-MacArthur: logistic prey + Holling II predation.
//   dx = r x (1 - x/K) - a x y / (1 + a h x)
//   dy = e a x y / (1 + a h x) - m y
inline Series rosenzweig_macarthur_rate(const Series &s, const Coeff &r,
                                        const Coeff &k, const Coeff &attack,
                                        const Coeff &handling,
                                        const Coeff &efficiency,
                                        const Coeff &mortality) {
  dynamics::require_species(s, 2, "rosenzweig_macarthur: state");
  const Series x = s.select(-1, 0);
  const Series y = s.select(-1, 1);
  const Series f = holling2(x, attack, handling);
  const Series dx = x.mul(r.on(x)).mul(x.div(k.on(x)).neg().add(1.0))
                        .sub(f.mul(y));
  const Series dy = efficiency.on(x).mul(f).mul(y).sub(mortality.on(x).mul(y));
  return dx.unsqueeze(-1).unite(std::vector<Series>{dy.unsqueeze(-1)}, -1);
}

} // namespace pluss::subsp::natsci::biology::population::interaction
