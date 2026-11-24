#pragma once

// Chemical / binding speciation: equilibrium concentrations of complexes from component totals.
//
//   components i = 1..m (free concentration x_i), complexes j = 1..c
//   c_j = K_j prod_i x_i^{N_ij}                       (mass-action, K_j = formation constant)
//   T_i = x_i + sum_j N_ij c_j                         (mass conservation, T_i > 0)
// Newton iteration on u = ln x (positivity by construction):
//   F(u) = x + N c - T,   dF/du = diag(x) + N diag(c) N^T   (symmetric positive definite),
// with a per-lane step cap |du|_inf <= max_step for global robustness. All lanes are solved in the
// same batched ops ([..., m, m] solve, [..., c] complexes); one convergence check per iteration.
// This is the stoichiometric analogue of the element-potential solve in chemistry::equilibrium and is
// shared by biology (ligand binding, allostery) and chemistry (complexation).

#include "pluss/subsp/natsci/common/dynamics.h"

#include <cmath>

namespace pluss::subsp::natsci::common::speciation {

namespace dynamics = ::pluss::subsp::natsci::common::dynamics;

struct Options final {
  int max_iterations = 200;
  double tolerance = 1e-12; // max_i |F_i| / T_i
  double max_step = 4.0;    // cap on |du| per iteration (log-concentration units)
};

struct Result final {
  Series free = pluss::series::undefined();      // [..., m]
  Series complexes = pluss::series::undefined(); // [..., c]
  Series residual = pluss::series::undefined();  // [...]
  int iterations = 0;
  bool converged = false; // every lane met opt.tolerance within opt.max_iterations
};

// stoichiometry [m, c]; ln_k [..., c]; totals [..., m]. A total is normally positive; a component such as H+ whose "total" is a
// proton excess may be zero or negative (a base), in which case the solution is found from a small positive starting guess.
// result.converged is false when max_iterations was reached before the tolerance.
inline Result solve(const Series &stoichiometry, const Series &ln_k, const Series &totals, const Options &opt = {}) {
  dynamics::require(stoichiometry.dim() == 2, "speciation: stoichiometry must be [components, complexes]");
  const std::int64_t m = stoichiometry.size(0);
  dynamics::require(totals.size(-1) == m, "speciation: totals must have one entry per component");
  dynamics::require(ln_k.size(-1) == stoichiometry.size(1), "speciation: ln_k must have one entry per complex");
  dynamics::require_finite(totals, "speciation: totals");
  dynamics::require_finite(ln_k, "speciation: ln_k");
  const Series n = stoichiometry.to(totals.scalar_type()).to(totals.platform());
  const Series nt = n.transpose(0, 1);
  const Config cfg = Config{}.with_dtype(totals.scalar_type()).with_platform(totals.platform());
  const Series eye = pluss::series::eye(m, cfg);

  // start from the total where it is positive, and from a small positive value where it is not (excess-of-reactant components)
  const Series magnitude = totals.abs().amax(-1, true).clamp_min(1e-300);
  const Series positive = totals.gt(0.0);
  Series u = pluss::series::where(positive, totals, magnitude.mul(1e-8)).log();
  Series x = totals, c = pluss::series::undefined(), resid = pluss::series::undefined();
  int it = 0;
  bool converged = false;
  for (; it < opt.max_iterations; ++it) {
    x = u.exp();
    c = ln_k.add(dynamics::matvec(nt, u)).exp();                       // [..., c]
    const Series f = x.add(dynamics::matvec(n, c)).sub(totals);         // [..., m]
    // relative balance error: against the total where it is positive, against the size of the terms otherwise
    const Series terms = x.add(dynamics::matvec(n.abs(), c));
    const Series scale = pluss::series::where(positive, totals, terms).clamp_min(1e-300);
    resid = f.div(scale).abs().amax(-1, false);
    if (resid.max().item<double>() < opt.tolerance) {
      converged = true;
      break;
    }
    const Series j = eye.mul(x.unsqueeze(-1)).add(n.mul(c.unsqueeze(-2)).matmul(nt));
    const Series du = dynamics::solve(j, f).neg();
    const Series stepscale = du.abs().amax(-1, true).clamp_min(opt.max_step).reciprocal().mul(opt.max_step);
    u = u.add(du.mul(stepscale));
  }
  return {x, c, resid, it, converged};
}

} // namespace pluss::subsp::natsci::common::speciation
