#pragma once

// Diffusion and reaction-diffusion on the shared periodic grid
// (common::spatial). Fields are [..., spatial...]; leading axes are batch
// lanes that evolve together (e.g. a sweep over D or r).
//
//   du/dt = D lap(u) + f(u)
//
// Exact references used in validation:
//  * Fisher-KPP  du/dt = D u_xx + r u (1 - u): minimum front speed
//    c = 2 sqrt(r D); Ablowitz-Zeppetella closed form
//    u = (1 + C exp((x - c t)/(sqrt(6)) sqrt(r/D)))^-2 at the special speed
//    c = 5/sqrt(6) sqrt(r D)   (Ablowitz & Zeppetella, Bull. Math. Biol. 41 (1979) 835; the speed and profile are restated in
//    Roques, arXiv:2511.10104 Thm 2.1, c_AZ = 5 sqrt(r0/6), read; the residual of the formula above vanishes identically under SymPy).
//    Front position from a step start: 2 sqrt(rD) t - 3/(2 sqrt(r/D)) log t + O(1) (Bramson, Mem. AMS 285 (1983), as quoted in
//    arXiv:1510.03329; the memoir itself was not obtained).
//  * Free-space heat kernel (4 pi D t)^(-d/2) exp(-|x|^2 / 4 D t).

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/common/field.h"

#include <functional>

namespace pluss::subsp::natsci::biology::biophysics::diffusion {

namespace spatial = ::pluss::subsp::natsci::common::spatial;
using dynamics::Coeff;

using Reaction = std::function<Series(const Series &)>;

inline Series diffusion_rate(const Series &u, const Coeff &d,
                             const spatial::PeriodicGrid &grid) {
  dynamics::require_positive(d, u, "diffusion: D");
  return spatial::laplacian(u, grid).mul(d.on(u));
}

inline Series reaction_diffusion_rate(const Series &u, const Coeff &d,
                                      const spatial::PeriodicGrid &grid,
                                      const Reaction &reaction) {
  return diffusion_rate(u, d, grid).add(reaction(u));
}

inline Series fisher_kpp_reaction(const Series &u, const Coeff &r) {
  return u.mul(r.on(u)).mul(u.neg().add(1.0));
}

inline double fisher_kpp_min_speed(double d, double r) {
  dynamics::require(d > 0.0 && r > 0.0, "fisher_kpp: D and r must be positive");
  return 2.0 * std::sqrt(d * r);
}

inline double fisher_kpp_exact_speed(double d, double r) {
  dynamics::require(d > 0.0 && r > 0.0, "fisher_kpp: D and r must be positive");
  return 5.0 / std::sqrt(6.0) * std::sqrt(d * r);
}

// Ablowitz-Zeppetella front, travelling right: 1 behind, 0 ahead.
inline Series fisher_kpp_exact(const Series &x, double t, double d, double r,
                               double shift = 1.0) {
  dynamics::require(shift > 0.0, "fisher_kpp: shift C must be positive");
  const double c = fisher_kpp_exact_speed(d, r);
  const double scale = std::sqrt(r / d) / std::sqrt(6.0);
  return x.sub(c * t).mul(scale).exp().mul(shift).add(1.0).pow(-2.0);
}

// Heat kernel at squared distance r2 for `dims` spatial dimensions.
inline Series heat_kernel(const Series &r2, const Coeff &d, const Coeff &t,
                          std::int64_t dims) {
  const Series four_dt = d.on(r2).mul(t.on(r2)).mul(4.0);
  return four_dt.mul(3.14159265358979323846).pow(-0.5 * static_cast<double>(dims))
      .mul(r2.div(four_dt).neg().exp());
}

} // namespace pluss::subsp::natsci::biology::biophysics::diffusion
