#pragma once

// Host-parasitoid and coupled-patch discrete-time models, batched over lanes.
//
//   Nicholson-Bailey (Nicholson & Bailey 1935)      H' = lambda H exp(-a P),         P' = c H (1 - exp(-a P))
//   density-dependent host (Beddington et al. 1975) H' = H exp(r (1 - H/K) - a P),   P' = c H (1 - exp(-a P))
//   aggregation, negative binomial (May 1978)        H' = lambda H (1 + a P / k)^-k,   P' = c H (1 - (1 + a P / k)^-k)
// Equilibria and local stability follow from the 2 x 2 Jacobian: the state is [..., 2] = (H, P); `jacobian` is analytic (checked against finite
// differences), `spectral_radius` is the modulus of the dominant eigenvalue (complex pairs included), stable iff < 1.
// Coupled-map lattice: n_i' = (1 - d) f(n_i) + d/2 (f(n_{i-1}) + f(n_{i+1})) on a ring (diffusive dispersal after local growth), the classic
// spatially extended chaos / synchrony testbed; `synchrony` returns the spatial standard deviation.
// The original papers were not obtained; results are checked against numerical eigenvalues, exact fixed points and conserved/closed-form limits.

#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <functional>

namespace pluss::subsp::natsci::biology::population::parasitoid {

using dynamics::Coeff;

inline Series nicholson_bailey(const Series &state, const Coeff &lambda, const Coeff &a, const Coeff &c) {
  const Series h = state.select(-1, 0), p = state.select(-1, 1);
  const Series escape = p.mul(a.on(p)).neg().exp();
  return h.mul(lambda.on(h)).mul(escape).unsqueeze(-1).unite(std::vector<Series>{h.mul(c.on(h)).mul(escape.neg().add(1.0)).unsqueeze(-1)}, state.dim() - 1);
}
inline Series beddington(const Series &state, const Coeff &r, const Coeff &k, const Coeff &a, const Coeff &c) {
  const Series h = state.select(-1, 0), p = state.select(-1, 1);
  const Series growth = h.div(k.on(h)).neg().add(1.0).mul(r.on(h)).sub(p.mul(a.on(p))).exp();
  const Series hit = p.mul(a.on(p)).neg().exp().neg().add(1.0);
  return h.mul(growth).unsqueeze(-1).unite(std::vector<Series>{h.mul(c.on(h)).mul(hit).unsqueeze(-1)}, state.dim() - 1);
}
inline Series negative_binomial(const Series &state, const Coeff &lambda, const Coeff &a, const Coeff &c, const Coeff &k) {
  const Series h = state.select(-1, 0), p = state.select(-1, 1);
  const Series escape = p.mul(a.on(p)).div(k.on(p)).add(1.0).pow(k.on(p).neg());
  return h.mul(lambda.on(h)).mul(escape).unsqueeze(-1).unite(std::vector<Series>{h.mul(c.on(h)).mul(escape.neg().add(1.0)).unsqueeze(-1)}, state.dim() - 1);
}

// interior equilibria; `like` carries the lane shape (a scalar for one parameter set), the result has a trailing axis (H*, P*)
inline Series nicholson_bailey_equilibrium(const Coeff &lambda, const Coeff &a, const Coeff &c, const Series &like) {
  const Series lam = lambda.on(like), p = lam.log().div(a.on(like));
  const Series h = p.mul(lam).div(c.on(like).mul(lam.sub(1.0)));
  return h.unsqueeze(-1).unite(std::vector<Series>{p.unsqueeze(-1)}, like.dim());
}
inline Series negative_binomial_equilibrium(const Coeff &lambda, const Coeff &a, const Coeff &c, const Coeff &k, const Series &like) {
  const Series lam = lambda.on(like), kk = k.on(like);
  const Series p = lam.pow(kk.reciprocal()).sub(1.0).mul(kk).div(a.on(like));          // (1 + aP/k)^k = lambda
  const Series h = p.mul(lam).div(c.on(like).mul(lam.sub(1.0)));
  return h.unsqueeze(-1).unite(std::vector<Series>{p.unsqueeze(-1)}, like.dim());
}

// Jacobian [..., 2, 2] of a state map by central differences in H and P (the maps above are smooth away from the axes)
inline Series jacobian(const std::function<Series(const Series &)> &map, const Series &state, double relative_step = 1e-6) {
  const Series step = state.abs().clamp(1e-12, 1e300).mul(relative_step);
  return dynamics::jacobian_scaled(map, state, step);
}
// modulus of the dominant eigenvalue of a 2 x 2 matrix a [..., 2, 2]
inline Series spectral_radius(const Series &a) {
  const Series tr = a.select(-1, 0).select(-1, 0).add(a.select(-1, 1).select(-1, 1));
  const Series det = a.select(-1, 0).select(-1, 0).mul(a.select(-1, 1).select(-1, 1)).sub(a.select(-1, 0).select(-1, 1).mul(a.select(-1, 1).select(-1, 0)));
  const Series disc = tr.square().sub(det.mul(4.0));
  const Series real_case = tr.abs().add(disc.clamp(0.0, 1e300).sqrt()).mul(0.5); // largest |root| of a real pair
  return disc.ge(0.0).where(real_case, det.sqrt());                               // complex pair: |lambda|^2 = det
}

// ---- coupled-map lattice on a ring ---------------------------------------------------------------------------------------------
inline Series ring_step(const std::function<Series(const Series &)> &local, const Series &n, double dispersal) {
  const Series f = local(n);
  const std::int64_t m = f.size(-1);
  const Series left = f.slice(-1, m - 1, m, 1).unite(std::vector<Series>{f.slice(-1, 0, m - 1, 1)}, f.dim() - 1);
  const Series right = f.slice(-1, 1, m, 1).unite(std::vector<Series>{f.slice(-1, 0, 1, 1)}, f.dim() - 1);
  return f.mul(1.0 - dispersal).add(left.add(right).mul(0.5 * dispersal));
}
inline Series synchrony(const Series &n) {
  const Series mean = n.mean(-1, true);
  return n.sub(mean).square().mean(-1, false).sqrt();
}

} // namespace pluss::subsp::natsci::biology::population::parasitoid
