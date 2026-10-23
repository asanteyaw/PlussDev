#pragma once

// Integral projection models (IPMs) and integrodifference equations, midpoint-rule discretised and batched over lanes.
//
//   n_{t+1}(z') = \int_L^U K(z', z) n_t(z) dz,   K = P + F,   P(z', z) = s(z) G(z' | z),   F(z', z) = f(z) D(z')
//
// Discretisation (Easterling, Ellner & Dixon, Ecology 81 (2000) 694 -- structure; Ellner & Rees, Am. Nat. 167 (2006) 410): n midpoints z_i,
// A_ij = h K(z_i, z_j). Neither paper was obtainable; the results below are verified against closed forms (a Gaussian-growth kernel with
// constant survival has lambda = c, w = the stationary AR(1) density, v = 1) and by midpoint-rule convergence.
//
//   lambda            dominant eigenvalue of A
//   w(z)              stable size density, \int w dz = 1
//   v(z)              reproductive value, \int v w dz = 1
//   s(z', z) = v(z') w(z)         sensitivity kernel:  d lambda = \iint s dK dz dz'  (~ h^2 sum s_ij dK_ij)
//   e(z', z) = K(z', z) s(z', z) / lambda     elasticity kernel, \iint e = 1
//   life expectancy eta(z) = ((I - P)^-1 1)(z) * h-free:  eta = 1^T (I - A_P)^-1,  R0 = rho(A_F (I - A_P)^-1)
//
// Integrodifference equations (Kot, Lewis & van den Driessche, Ecology 77 (1996) 2027, not obtained): n_{t+1}(x) = \int k(x - y) g(n_t(y)) dy
// with R0 = g'(0); the asymptotic spreading speed is c* = min_{s > 0} ln(R0 M(s)) / s with M(s) the moment generating function of the
// dispersal kernel. Gaussian dispersal gives c* = sigma sqrt(2 ln R0) exactly.

#include "pluss/subsp/linearalg/linearalg.h"
#include "pluss/subsp/natsci/biology/population/structured.h"

#include <cmath>
#include <functional>

namespace pluss::subsp::natsci::biology::population::ipm {

namespace structured = ::pluss::subsp::natsci::biology::population::structured;

struct Mesh final {
  Series nodes = pluss::series::undefined(); // [n] midpoints
  double h{0.0};
  double lower{0.0}, upper{0.0};
};
inline Mesh mesh(double lower, double upper, std::int64_t n, const Config &cfg = Config{}.with_dtype(pluss::ScalarType::Float64)) {
  dynamics::require(upper > lower && n >= 2, "ipm::mesh: need upper > lower and at least two points");
  const double h = (upper - lower) / static_cast<double>(n);
  return {pluss::series::arange(0, n, 1, cfg.dtype).mul(h).add(lower + 0.5 * h), h, lower, upper};
}

using Kernel = std::function<Series(const Series &zprime, const Series &z)>; // zprime [n, 1], z [1, n] -> [n, n] (or batched leading axes)

// h K(z_i, z_j)
inline Series discretize(const Kernel &k, const Mesh &m) {
  return k(m.nodes.unsqueeze(-1), m.nodes.unsqueeze(0)).mul(m.h);
}

inline Series gaussian_density(const Series &x, const Series &mean, const Series &sd) {
  const Series z = x.sub(mean).div(sd);
  return z.square().mul(-0.5).exp().div(sd.mul(std::sqrt(2.0 * M_PI)));
}

struct Analysis final {
  Series lambda = pluss::series::undefined();      // [...]
  Series w = pluss::series::undefined();           // [..., n], integral 1
  Series v = pluss::series::undefined();           // [..., n], integral of v w = 1
  Series sensitivity = pluss::series::undefined(); // [..., n, n]  s(z', z) = v(z') w(z)
  Series elasticity = pluss::series::undefined();  // [..., n, n]  (needs the kernel: A / h)
  bool converged{false};
};
inline Analysis analyze(const Series &a, double h) {
  const structured::Eigen e = structured::dominant(a);
  Analysis out;
  out.lambda = e.lambda;
  out.w = e.right.div(e.right.sum(-1, true).mul(h));                   // h sum w = 1
  const Series vv = e.left.div(e.left.mul(e.right).sum(-1, true));      // sum v w_right = 1 with w_right summing to 1
  // continuous normalisation: h sum v w = 1
  out.v = vv.div(vv.mul(out.w).sum(-1, true).mul(h));
  out.sensitivity = out.v.unsqueeze(-1).mul(out.w.unsqueeze(-2));
  out.elasticity = a.div(h).mul(out.sensitivity).div(out.lambda.unsqueeze(-1).unsqueeze(-1));
  out.converged = e.converged;
  return out;
}

// life expectancy eta_j = sum_i [(I - A_P)^-1]_ij (time spent in the state space before leaving it), and R0 = rho(A_F (I - A_P)^-1)
inline Series life_expectancy(const Series &survival_growth) {
  const Config cfg = Config{}.with_dtype(survival_growth.scalar_type()).with_platform(survival_growth.platform());
  return pluss::subsp::linearalg::inverse(pluss::series::eye(survival_growth.size(-1), cfg).sub(survival_growth)).sum(-2, false);
}
inline Series net_reproductive_rate(const Series &survival_growth, const Series &fecundity) {
  const Config cfg = Config{}.with_dtype(survival_growth.scalar_type()).with_platform(survival_growth.platform());
  const Series n = pluss::subsp::linearalg::inverse(pluss::series::eye(survival_growth.size(-1), cfg).sub(survival_growth));
  return structured::dominant(fecundity.matmul(n)).lambda;
}

// \int z n(z) dz / \int n dz and the variance, for a density n [..., n_nodes] on the mesh
inline Series mean_size(const Series &density, const Mesh &m) { return density.mul(m.nodes).sum(-1, false).div(density.sum(-1, false)); }
inline Series variance_size(const Series &density, const Mesh &m) {
  const Series mu = mean_size(density, m);
  return density.mul(m.nodes.sub(mu.unsqueeze(-1)).square()).sum(-1, false).div(density.sum(-1, false));
}

// ---- spreading speed of an integrodifference equation ---------------------------------------------------------------------------
// c* = min_{0 < s < s_max} ln(R0 M(s)) / s by golden-section search, all lanes at once (ln_r0 any shape, `mgf` elementwise in s)
inline Series spreading_speed(const Series &ln_r0, const std::function<Series(const Series &)> &mgf, double s_max, int iterations = 200) {
  const double phi = (std::sqrt(5.0) - 1.0) / 2.0;
  Series a = ln_r0.mul(0.0).add(1e-6 * s_max), b = ln_r0.mul(0.0).add(s_max * (1.0 - 1e-9));
  const auto f = [&](const Series &s) { return ln_r0.add(mgf(s).log()).div(s); };
  Series c = b.sub(b.sub(a).mul(phi)), d = a.add(b.sub(a).mul(phi));
  Series fc = f(c), fd = f(d);
  for (int i = 0; i < iterations; ++i) {
    const Series left = fc.lt(fd);
    b = left.where(d, b);
    a = left.where(a, c);
    c = b.sub(b.sub(a).mul(phi));
    d = a.add(b.sub(a).mul(phi));
    fc = f(c);
    fd = f(d);
  }
  return f(a.add(b).mul(0.5));
}

} // namespace pluss::subsp::natsci::biology::population::ipm
