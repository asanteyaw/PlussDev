#pragma once

// Harvesting, migration and metapopulation dynamics. All functions are Series-native and
// batched; thresholds are found with the shared solvers (math/roots.h) and the shared
// stability tools (common/stability.h), and the metapopulation persistence threshold reuses
// the dominant-eigenvalue routine of structured.h.
//
//  Harvesting (Schaefer / constant yield; e.g. Brauer & Castillo-Chavez, Mathematical Models
//  in Population Biology and Epidemiology, ch. 1-2): 
//     effort:  dn = r n (1 - n/K) - q E n,   n* = K (1 - qE/r),  yield q E n*,  MSY = rK/4 at E = r/(2q)
//     yield:   dn = r n (1 - n/K) - h,       n_pm = K/2 (1 +- sqrt(1 - 4h/(rK))), saddle-node at h = rK/4
//  Levins (1969) metapopulation: dp = c p (1 - p) - e p, p* = 1 - e/c; with the rescue effect
//     dp = c p (1 - p) - e p (1 - p).
//  Patch model (Hanski & Ovaskainen 2000 form): dp_i = (sum_j C_ij p_j)(1 - p_i) - e_i p_i; the
//     empty state is unstable iff the dominant eigenvalue of C exceeds e (equal extinction rates).
//  Migration: dn_i = sum_j (m_ji n_j - m_ij n_i) for a rate matrix m (m_ij = rate from i to j);
//     it conserves total abundance.

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/biology/population/growth.h"
#include "pluss/subsp/natsci/biology/population/structured.h"

namespace pluss::subsp::natsci::biology::population::ecology {

using dynamics::Coeff;

inline Series effort_harvest_rate(const Series &n, const Coeff &r, const Coeff &k, const Coeff &catchability,
                                  const Coeff &effort) {
  return growth::logistic_rate(n, r, k).sub(catchability.on(n).mul(effort.on(n)).mul(n));
}
inline Series effort_equilibrium(const Coeff &r, const Coeff &k, const Coeff &catchability, const Coeff &effort,
                                 const Series &like) {
  return k.on(like).mul(catchability.on(like).mul(effort.on(like)).div(r.on(like)).neg().add(1.0));
}
// Sustainable yield q E n* at equilibrium effort E.
inline Series effort_yield(const Coeff &r, const Coeff &k, const Coeff &catchability, const Coeff &effort,
                           const Series &like) {
  return catchability.on(like).mul(effort.on(like)).mul(effort_equilibrium(r, k, catchability, effort, like));
}
inline Series maximum_sustainable_yield(const Coeff &r, const Coeff &k, const Series &like) {
  return r.on(like).mul(k.on(like)).mul(0.25);
}

inline Series constant_yield_rate(const Series &n, const Coeff &r, const Coeff &k, const Coeff &h) {
  return growth::logistic_rate(n, r, k).sub(h.on(n));
}
// Larger (stable) and smaller (unstable) equilibria; NaN-free: returns zeros where h > rK/4 (collapse).
struct YieldEquilibria final {
  Series stable = pluss::series::undefined();
  Series unstable = pluss::series::undefined();
  Series exists = pluss::series::undefined(); // 1 where h <= rK/4
};
inline YieldEquilibria constant_yield_equilibria(const Coeff &r, const Coeff &k, const Coeff &h, const Series &like) {
  const Series kk = k.on(like), rr = r.on(like), hh = h.on(like);
  const Series disc = hh.mul(4.0).div(rr.mul(kk)).neg().add(1.0);
  const Series ok = disc.ge(0.0);
  const Series root = ok.where(disc, disc.mul(0.0)).sqrt();
  return {kk.mul(0.5).mul(root.add(1.0)), kk.mul(0.5).mul(root.neg().add(1.0)), ok.to(like.scalar_type())};
}

inline Series levins_rate(const Series &p, const Coeff &colonisation, const Coeff &extinction, bool rescue = false) {
  const Series gain = colonisation.on(p).mul(p).mul(p.neg().add(1.0));
  const Series loss = rescue ? extinction.on(p).mul(p).mul(p.neg().add(1.0)) : extinction.on(p).mul(p);
  return gain.sub(loss);
}
inline Series levins_equilibrium(const Coeff &colonisation, const Coeff &extinction, const Series &like) {
  // broadcast to the shape of `like` (scalar coefficients are rank 0)
  return like.mul(0.0).add(1.0).sub(extinction.on(like).div(colonisation.on(like))).clamp(0.0, 1.0);
}

// Explicit patch network. p [..., P], connectivity C [P, P] (non-negative, C_ii = 0 by convention),
// extinction e [..., P] or scalar.
inline Series patch_occupancy_rate(const Series &p, const Series &connectivity, const Coeff &extinction) {
  dynamics::require_square(connectivity, "patch_occupancy: connectivity");
  dynamics::require_species(p, connectivity.size(-1), "patch_occupancy: state");
  const Series colonise = dynamics::matvec(connectivity, p).mul(p.neg().add(1.0));
  return colonise.sub(extinction.on(p).mul(p));
}
// Metapopulation capacity (dominant eigenvalue of the connectivity matrix, Hanski & Ovaskainen 2000):
// persistence needs lambda_M > e for equal extinction rates e.
inline Series metapopulation_capacity(const Series &connectivity) {
  return structured::dominant(connectivity).lambda;
}

inline Series migration_rate(const Series &n, const Series &rates) {
  dynamics::require_square(rates, "migration: rate matrix");
  dynamics::require_species(n, rates.size(-1), "migration: state");
  const Series inflow = dynamics::matvec(rates.transpose(-1, -2), n);  // sum_j m_ji n_j
  const Series outflow = n.mul(rates.sum(-1, false));                  // n_i sum_j m_ij
  return inflow.sub(outflow);
}

} // namespace pluss::subsp::natsci::biology::population::ecology
