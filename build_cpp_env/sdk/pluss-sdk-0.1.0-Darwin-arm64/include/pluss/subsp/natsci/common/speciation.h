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
};

// stoichiometry [m, c]; ln_k [..., c]; totals [..., m] (> 0).
inline Result solve(const Series &stoichiometry, const Series &ln_k, const Series &totals, const Options &opt = {}) {
  dynamics::require(stoichiometry.dim() == 2, "speciation: stoichiometry must be [components, complexes]");
  const std::int64_t m = stoichiometry.size(0);
  dynamics::require(totals.size(-1) == m, "speciation: totals must have one entry per component");
  dynamics::require(ln_k.size(-1) == stoichiometry.size(1), "speciation: ln_k must have one entry per complex");
  dynamics::require_positive(totals, "speciation: totals");
  dynamics::require_finite(ln_k, "speciation: ln_k");
  const Series n = stoichiometry.to(totals.scalar_type()).to(totals.platform());
  const Series nt = n.transpose(0, 1);
  const Config cfg = Config{}.with_dtype(totals.scalar_type()).with_platform(totals.platform());
  const Series eye = pluss::series::eye(m, cfg);

  Series u = totals.log();
  Series x = totals, c = pluss::series::undefined(), resid = pluss::series::undefined();
  int it = 0;
  for (; it < opt.max_iterations; ++it) {
    x = u.exp();
    c = ln_k.add(dynamics::matvec(nt, u)).exp();                       // [..., c]
    const Series f = x.add(dynamics::matvec(n, c)).sub(totals);         // [..., m]
    resid = f.div(totals).abs().amax(-1, false);
    if (resid.max().item<double>() < opt.tolerance)
      break;
    // J = diag(x) + N diag(c) N^T  -> [..., m, m] from one batched matmul
    const Series j = eye.mul(x.unsqueeze(-1)).add(n.mul(c.unsqueeze(-2)).matmul(nt));
    const Series du = dynamics::solve(j, f).neg();
    const Series scale = du.abs().amax(-1, true).clamp_min(opt.max_step).reciprocal().mul(opt.max_step);
    u = u.add(du.mul(scale));
  }
  return {x, c, resid, it};
}

} // namespace pluss::subsp::natsci::common::speciation
