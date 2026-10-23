#pragma once

// Diffusion-driven (Turing) instability of a homogeneous steady state u* of  du/dt = D lap(u) + f(u)  and the standard pattern-forming
// reaction kinetics, batched over lanes of parameters.
//
// A perturbation exp(i k.x) grows with the eigenvalues lambda(k^2) of  M(k^2) = J - k^2 D,  J = df/du at u*. For two species with
// J = [[fu, fv], [gu, gv]] and D = diag(da, db) (Turing 1952; Murray, Mathematical Biology II, ch. 2):
//   stable without diffusion:   fu + gv < 0,  det J = fu gv - fv gu > 0
//   unstable with diffusion:    da gv + db fu > 0   and   (da gv + db fu)^2 > 4 da db det J
// the unstable band is  k^2 in (k_-^2, k_+^2),  k_pm^2 = [ (da gv + db fu) +- sqrt((da gv + db fu)^2 - 4 da db det J) ] / (2 da db),
// i.e. the roots of h(k^2) = da db k^4 - (da gv + db fu) k^2 + det J = det M(k^2), and the critical wavenumber at onset is
// k_c^2 = (da gv + db fu)/(2 da db) = sqrt(det J/(da db)). For S > 2 species the unstable-eigenvalue count of M(k^2) comes from the
// Routh-Hurwitz machinery of common/stability.h.

#include "pluss/subsp/natsci/biology/dynamics.h"
#include "pluss/subsp/natsci/common/stability.h"

#include <cmath>

namespace pluss::subsp::natsci::biology::spatial::turing {

namespace stability = ::pluss::subsp::natsci::common::stability;

// ---- two-species analysis ----------------------------------------------------------------------------------------------------
struct Jacobian2 final {
  Series fu = pluss::series::undefined(), fv = pluss::series::undefined(), gu = pluss::series::undefined(), gv = pluss::series::undefined();
};
inline Series determinant(const Jacobian2 &j) { return j.fu.mul(j.gv).sub(j.fv.mul(j.gu)); }

// largest real part of the eigenvalues of J - k2 D (k2 any shape broadcasting against the Jacobian entries)
inline Series growth_rate(const Jacobian2 &j, const Series &da, const Series &db, const Series &k2) {
  const Series a = j.fu.sub(da.mul(k2)), d = j.gv.sub(db.mul(k2));
  const Series tr = a.add(d), det = a.mul(d).sub(j.fv.mul(j.gu));
  const Series disc = tr.square().sub(det.mul(4.0));
  const Series root = disc.clamp(0.0, 1e300).sqrt();
  const Series real_case = tr.add(root).mul(0.5);        // disc >= 0: larger real eigenvalue
  const Series complex_case = tr.mul(0.5);               // disc < 0: real part of the complex pair
  return disc.ge(0.0).where(real_case, complex_case);
}
// dispersion h(k^2) = det(J - k^2 D)
inline Series determinant_of_perturbation(const Jacobian2 &j, const Series &da, const Series &db, const Series &k2) {
  return j.fu.sub(da.mul(k2)).mul(j.gv.sub(db.mul(k2))).sub(j.fv.mul(j.gu));
}
struct Conditions final {
  Series stable_without_diffusion = pluss::series::undefined(); // Bool
  Series turing_unstable = pluss::series::undefined();          // Bool: all three conditions hold
  Series k2_critical = pluss::series::undefined();              // (da gv + db fu) / (2 da db)
  Series k2_lower = pluss::series::undefined();                 // band edges (NaN where there is no band)
  Series k2_upper = pluss::series::undefined();
};
inline Conditions turing_conditions(const Jacobian2 &j, const Series &da, const Series &db) {
  const Series det = determinant(j), tr = j.fu.add(j.gv);
  const Series m = da.mul(j.gv).add(db.mul(j.fu));
  const Series disc = m.square().sub(da.mul(db).mul(det).mul(4.0));
  Conditions out;
  out.stable_without_diffusion = tr.lt(0.0).logical_and(det.gt(0.0));
  out.turing_unstable = out.stable_without_diffusion.logical_and(m.gt(0.0)).logical_and(disc.gt(0.0));
  out.k2_critical = m.div(da.mul(db).mul(2.0));
  const Series root = disc.clamp(0.0, 1e300).sqrt();
  const Series two = da.mul(db).mul(2.0);
  const Series nan = m.mul(0.0).add(std::numeric_limits<double>::quiet_NaN());
  out.k2_lower = out.turing_unstable.where(m.sub(root).div(two), nan);
  out.k2_upper = out.turing_unstable.where(m.add(root).div(two), nan);
  return out;
}

// ---- n species ------------------------------------------------------------------------------------------------------------------
// number of eigenvalues of J - k2 D with positive real part; J [..., S, S], d [S] or [..., S], k2 [...]
inline Series unstable_modes(const Series &jac, const Series &diffusion, const Series &k2) {
  const Config cfg = Config{}.with_dtype(jac.scalar_type()).with_platform(jac.platform());
  const Series eye = pluss::series::eye(jac.size(-1), cfg);
  const Series d = diffusion.unsqueeze(-1).mul(eye);
  return stability::unstable_modes(jac.sub(d.mul(k2.unsqueeze(-1).unsqueeze(-1)))).unstable;
}

// ---- standard kinetics ----------------------------------------------------------------------------------------------------------
// reactions act on states [..., 2, ...] (species axis at position `species_axis` from the front of the spatial block; default: axis -(dim+1))
inline Series component(const Series &u, std::int64_t species, std::int64_t spatial_dim) { return u.select(u.dim() - spatial_dim - 1, species); }
inline Series stack2(const Series &a, const Series &b, std::int64_t spatial_dim) {
  const std::int64_t axis = a.dim() - spatial_dim; // new species axis sits before the spatial axes
  return a.unsqueeze(axis).unite(std::vector<Series>{b.unsqueeze(axis)}, axis);
}

// Brusselator (Prigogine & Lefever 1968): u' = a - (b + 1) u + u^2 v,  v' = b u - u^2 v ; steady state (a, b/a);
// Turing-Hopf analysis: Hopf at b = 1 + a^2, Turing band exists for b > (1 + a sqrt(da/db))^2 when da < db ... (derived from the conditions above)
struct Brusselator final {
  double a{1.0}, b{3.0};
  [[nodiscard]] Series reaction(const Series &u, std::int64_t sd) const {
    const Series x = component(u, 0, sd), y = component(u, 1, sd);
    const Series x2y = x.square().mul(y);
    return stack2(x.mul(-(b + 1.0)).add(a).add(x2y), x.mul(b).sub(x2y), sd);
  }
  [[nodiscard]] std::pair<double, double> steady_state() const { return {a, b / a}; }
  [[nodiscard]] Jacobian2 jacobian(const Series &like) const {
    const auto mk = [&](double v) { return like.mul(0.0).add(v); };
    return {mk(b - 1.0), mk(a * a), mk(-b), mk(-a * a)};
  }
};
// Schnakenberg (1979): u' = a - u + u^2 v,  v' = b - u^2 v ; steady state (a + b, b/(a + b)^2)
struct Schnakenberg final {
  double a{0.1}, b{0.9};
  [[nodiscard]] Series reaction(const Series &u, std::int64_t sd) const {
    const Series x = component(u, 0, sd), y = component(u, 1, sd);
    const Series x2y = x.square().mul(y);
    return stack2(x.mul(-1.0).add(a).add(x2y), x2y.mul(-1.0).add(b), sd);
  }
  [[nodiscard]] std::pair<double, double> steady_state() const { return {a + b, b / ((a + b) * (a + b))}; }
  [[nodiscard]] Jacobian2 jacobian(const Series &like) const {
    const auto mk = [&](double v) { return like.mul(0.0).add(v); };
    const double s = a + b;
    return {mk((b - a) / s), mk(s * s), mk(-2.0 * b / s), mk(-s * s)};
  }
};
// Gray-Scott (Gray & Scott 1984; Pearson, Science 261 (1993) 189): u' = -u v^2 + F (1 - u),  v' = u v^2 - (F + k) v.
// Steady states: (1, 0) and, when 4 (F + k)^2 <= F, u = [1 +- sqrt(1 - 4 (F + k)^2 / F)] / 2, v = F (1 - u) / (F + k).
struct GrayScott final {
  double feed{0.037}, kill{0.06};
  [[nodiscard]] Series reaction(const Series &u, std::int64_t sd) const {
    const Series x = component(u, 0, sd), y = component(u, 1, sd);
    const Series uv2 = x.mul(y.square());
    return stack2(uv2.mul(-1.0).add(x.mul(-1.0).add(1.0).mul(feed)), uv2.sub(y.mul(feed + kill)), sd);
  }
  [[nodiscard]] std::pair<double, double> nontrivial_steady_state(bool upper_root) const {
    const double disc = 1.0 - 4.0 * (feed + kill) * (feed + kill) / feed;
    dynamics::require(disc >= 0.0, "GrayScott: no non-trivial steady state (4 (F + k)^2 > F)");
    const double u = 0.5 * (1.0 + (upper_root ? 1.0 : -1.0) * std::sqrt(disc));
    return {u, feed * (1.0 - u) / (feed + kill)};
  }
  [[nodiscard]] Jacobian2 jacobian_at(double u, double v, const Series &like) const {
    const auto mk = [&](double x) { return like.mul(0.0).add(x); };
    return {mk(-v * v - feed), mk(-2.0 * u * v), mk(v * v), mk(2.0 * u * v - (feed + kill))};
  }
};

} // namespace pluss::subsp::natsci::biology::spatial::turing
