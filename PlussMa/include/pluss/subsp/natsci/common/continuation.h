#pragma once

// Numerical continuation of equilibrium branches  G(u, p) = 0  (u in R^n, one parameter p) with detection of the codimension-one
// bifurcations of an ODE  du/dt = G(u, p):
//
//   * pseudo-arclength continuation (Keller 1977): tangent from  [G_u G_p; t_prev^T] t = e_{n+1}, Euler predictor, Newton corrector
//     on the bordered system  [G(z); t^T (z - z_pred)] = 0, z = (u, p). Follows the branch around folds; the step is adapted
//     to Newton's convergence rate.
//   * fold (saddle-node): the p-component of the tangent changes sign;
//     branch point (transcritical / pitchfork): the determinant of the bordered matrix [G_u G_p; t^T] changes sign
//     (it does not vanish at a fold, unlike det G_u);
//     Hopf: the number of eigenvalues of G_u with positive real part changes by two while det G_u keeps its sign
//     (Routh-Hurwitz count from stability.h; no eigensolver is needed). Every detected point is refined by bisection on the arclength
//     with the Newton corrector, and the Hopf frequency is the root of the characteristic polynomial on the imaginary axis.
//   * branch switching at a branch point: the directions of the emanating branches solve the algebraic bifurcation equation
//     c11 a^2 + 2 c12 a b + c22 b^2 = 0 built from second directional derivatives of G along the tangent and the second null vector
//     (Seydel 1991; Kuznetsov, Elements of Applied Bifurcation Theory, 3rd ed., sec. 10.2).
//
// Every derivative is a central difference whose 2n + 3 evaluation points go through the residual in ONE batched call, so the residual
// must treat the leading axis as independent lanes:  G(u [K, n], p [K]) -> [K, n]. The (n + 1)-dimensional bordered linear algebra
// runs on the host (it is tiny and needs pivoting and determinant signs); everything model-sized is batched Series work.

#include "pluss/subsp/natsci/common/dynamics.h"
#include "pluss/subsp/natsci/common/stability.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace pluss::subsp::natsci::common::continuation {

using Residual = std::function<Series(const Series &u, const Series &p)>; // u [K, n], p [K] -> [K, n]

enum class PointKind { Fold, Hopf, BranchPoint };

inline const char *to_string(PointKind k) {
  switch (k) {
  case PointKind::Fold: return "fold";
  case PointKind::Hopf: return "hopf";
  case PointKind::BranchPoint: return "branch point";
  }
  return "?";
}

struct SpecialPoint final {
  PointKind kind{PointKind::Fold};
  std::int64_t index{0};  // first accepted branch point after the event
  double p{0.0};
  std::vector<double> u;
  double omega{0.0};      // Hopf: imaginary part of the critical pair
};

struct Options final {
  double step = 0.05;       // initial arclength step
  double step_min = 1e-7;
  double step_max = 0.5;
  std::int64_t max_points = 2000;
  int max_newton = 10;
  double tolerance = 1e-10; // residual / correction tolerance of the corrector
  double p_min = -std::numeric_limits<double>::infinity();
  double p_max = std::numeric_limits<double>::infinity();
  double fd_step = 1e-6;    // relative central-difference step
  bool detect = true;
  bool stability = true;    // count unstable eigenvalues (and detect Hopf); needs G to be a vector field
  std::vector<double> u_scale; // optional per-component weights of the arclength norm (default 1); p has weight 1
};

struct Branch final {
  Series u = pluss::series::undefined();        // [N, n]
  Series p = pluss::series::undefined();        // [N]
  Series unstable = pluss::series::undefined(); // [N] eigenvalues of G_u with Re > 0 (0 when stability is off)
  std::vector<SpecialPoint> special;
  std::int64_t points{0};
  std::string stopped; // "p_min/p_max reached", "max_points", "step too small", "closed"
};

namespace detail {

using Vec = std::vector<double>;

inline Series column(const Vec &v, std::int64_t k, std::int64_t n) {
  return pluss::make_series(v, {k, n}).to(pluss::ScalarType::Float64);
}

// Dense row-major LU with partial pivoting: solves A x = b and returns det(A) (0 if singular).
inline double lu_solve(Vec a, Vec b, std::size_t n, Vec &x, bool want_solution = true) {
  double det = 1.0;
  for (std::size_t k = 0; k < n; ++k) {
    std::size_t piv = k;
    for (std::size_t i = k + 1; i < n; ++i)
      if (std::abs(a[i * n + k]) > std::abs(a[piv * n + k]))
        piv = i;
    if (a[piv * n + k] == 0.0) {
      x.assign(n, 0.0);
      return 0.0;
    }
    if (piv != k) {
      for (std::size_t j = 0; j < n; ++j)
        std::swap(a[k * n + j], a[piv * n + j]);
      std::swap(b[k], b[piv]);
      det = -det;
    }
    det *= a[k * n + k];
    for (std::size_t i = k + 1; i < n; ++i) {
      const double f = a[i * n + k] / a[k * n + k];
      if (f == 0.0)
        continue;
      for (std::size_t j = k; j < n; ++j)
        a[i * n + j] -= f * a[k * n + j];
      b[i] -= f * b[k];
    }
  }
  if (want_solution) {
    x.assign(n, 0.0);
    for (std::size_t ii = n; ii-- > 0;) {
      double s = b[ii];
      for (std::size_t j = ii + 1; j < n; ++j)
        s -= a[ii * n + j] * x[j];
      x[ii] = s / a[ii * n + ii];
    }
  }
  return det;
}

inline double dot(const Vec &a, const Vec &b) {
  double s = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i)
    s += a[i] * b[i];
  return s;
}

struct Context final {
  const Residual *g{};
  std::int64_t n{0};
  Options opt;
  Vec w; // arclength weights, size n + 1

  [[nodiscard]] double norm(const Vec &v) const {
    double s = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i)
      s += w[i] * w[i] * v[i] * v[i];
    return std::sqrt(s);
  }
  // weighted inner product consistent with norm()
  [[nodiscard]] double wdot(const Vec &a, const Vec &b) const {
    double s = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
      s += w[i] * w[i] * a[i] * b[i];
    return s;
  }
};

struct Lin final {
  Vec g;  // [n]
  Vec jac; // [n x (n + 1)] row-major: columns 0..n-1 = G_u, column n = G_p
};

// G and its Jacobian at z = (u, p) from ONE batched residual call with 2n + 3 lanes.
inline Lin linearize(const Context &c, const Vec &z) {
  const auto n = static_cast<std::size_t>(c.n);
  const std::size_t lanes = 2 * n + 3;
  Vec U(lanes * n), P(lanes), h(n + 1);
  for (std::size_t j = 0; j <= n; ++j)
    h[j] = c.opt.fd_step * std::max(1.0, std::abs(z[j]));
  for (std::size_t k = 0; k < lanes; ++k) {
    for (std::size_t i = 0; i < n; ++i)
      U[k * n + i] = z[i];
    P[k] = z[n];
  }
  for (std::size_t j = 0; j < n; ++j) {
    U[j * n + j] += h[j];
    U[(n + j) * n + j] -= h[j];
  }
  P[2 * n] += h[n];
  P[2 * n + 1] -= h[n];
  const Series out = (*c.g)(column(U, static_cast<std::int64_t>(lanes), c.n), pluss::make_series(P, {static_cast<std::int64_t>(lanes)}).to(pluss::ScalarType::Float64));
  const Vec f = out.contiguous().host<double>().array().data;
  Lin l;
  l.g.assign(f.begin() + static_cast<std::ptrdiff_t>(2 * n + 2) * static_cast<std::ptrdiff_t>(n) , f.begin() + static_cast<std::ptrdiff_t>(2 * n + 3) * static_cast<std::ptrdiff_t>(n));
  l.jac.assign(n * (n + 1), 0.0);
  for (std::size_t j = 0; j <= n; ++j) {
    const std::size_t kp = j < n ? j : 2 * n, km = j < n ? n + j : 2 * n + 1;
    for (std::size_t i = 0; i < n; ++i)
      l.jac[i * (n + 1) + j] = (f[kp * n + i] - f[km * n + i]) / (2.0 * h[j]);
  }
  return l;
}

// Matrix [J; t^T] (row-major (n + 1)^2)
inline Vec bordered(const Lin &l, const Vec &t, std::size_t n) {
  Vec m((n + 1) * (n + 1));
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j <= n; ++j)
      m[i * (n + 1) + j] = l.jac[i * (n + 1) + j];
  for (std::size_t j = 0; j <= n; ++j)
    m[n * (n + 1) + j] = t[j];
  return m;
}

// Unit tangent (weighted norm) of the branch at z given a reference direction for orientation.
inline Vec tangent(const Context &c, const Lin &l, const Vec &ref, double *bordered_det = nullptr) {
  const auto n = static_cast<std::size_t>(c.n);
  Vec rhs(n + 1, 0.0), t;
  rhs[n] = 1.0;
  // weighted bordering: the last row is w^2 * ref so that t . (w^2 ref) = 1 normalises in the arclength metric
  Vec wref(n + 1);
  for (std::size_t i = 0; i <= n; ++i)
    wref[i] = c.w[i] * c.w[i] * ref[i];
  const double det = lu_solve(bordered(l, wref, n), rhs, n + 1, t);
  if (bordered_det)
    *bordered_det = det;
  const double nt = c.norm(t);
  for (auto &v : t)
    v /= nt;
  if (c.wdot(t, ref) < 0.0)
    for (auto &v : t)
      v = -v;
  return t;
}

struct Corrected final {
  Vec z;
  Vec t;       // tangent at the corrected point
  Lin lin;
  double det{0.0}; // det of the bordered matrix [G_u G_p; t^T] with t oriented along the branch
  bool ok{false};
  int iterations{0};
};

// Newton corrector on  [G(z); t0 . (z - z_pred)] = 0  (t0 in the weighted metric).
inline Corrected correct(const Context &c, const Vec &z_pred, const Vec &t0) {
  const auto n = static_cast<std::size_t>(c.n);
  Corrected r;
  r.z = z_pred;
  Vec wt(n + 1);
  for (std::size_t i = 0; i <= n; ++i)
    wt[i] = c.w[i] * c.w[i] * t0[i];
  for (int it = 0; it < c.opt.max_newton; ++it) {
    r.lin = linearize(c, r.z);
    Vec rhs(n + 1), d;
    double gmax = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      rhs[i] = -r.lin.g[i];
      gmax = std::max(gmax, std::abs(r.lin.g[i]));
    }
    Vec dz(n + 1);
    for (std::size_t i = 0; i <= n; ++i)
      dz[i] = r.z[i] - z_pred[i];
    rhs[n] = -dot(wt, dz);
    r.iterations = it;
    const double det = lu_solve(bordered(r.lin, wt, n), rhs, n + 1, d);
    if (det == 0.0 || !std::isfinite(d[0]))
      return r;
    double dmax = 0.0, zmax = 1.0;
    for (std::size_t i = 0; i <= n; ++i) {
      r.z[i] += d[i];
      dmax = std::max(dmax, std::abs(d[i]));
      zmax = std::max(zmax, std::abs(r.z[i]));
    }
    if (dmax < c.opt.tolerance * zmax && gmax < 1e3 * c.opt.tolerance * zmax) {
      r.lin = linearize(c, r.z);
      r.t = tangent(c, r.lin, t0, &r.det);
      // bordered determinant with the oriented tangent
      r.det = lu_solve(bordered(r.lin, r.t, n), Vec(n + 1, 0.0), n + 1, d, false);
      r.ok = true;
      r.iterations = it + 1;
      return r;
    }
  }
  return r;
}

inline std::vector<double> sub_u(const Vec &z, std::size_t n) { return Vec(z.begin(), z.begin() + static_cast<std::ptrdiff_t>(n)); }

// (count of eigenvalues of G_u with Re > 0, det G_u, coefficients of the characteristic polynomial)
struct Spectrum final {
  int unstable{0};
  double det_gu{0.0};
  Vec poly; // c_0 .. c_n of det(lambda I - G_u)
};

inline Spectrum spectrum(const Context &c, const Lin &l) {
  const auto n = static_cast<std::size_t>(c.n);
  Vec a(n * n);
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j)
      a[i * n + j] = l.jac[i * (n + 1) + j];
  const Series A = pluss::make_series(a, {c.n, c.n}).to(pluss::ScalarType::Float64);
  const Series poly = stability::characteristic_polynomial(A);
  Spectrum s;
  s.poly = poly.contiguous().host<double>().array().data;
  s.unstable = static_cast<int>(std::lround(stability::routh(poly).unstable.item<double>()));
  Vec x;
  s.det_gu = lu_solve(a, Vec(n, 0.0), n, x, false);
  return s;
}

// Root of det(i w I - A) = 0 with w > 0 nearest to the minimum of |p(i w)| on a logarithmic grid, polished by Newton.
inline double hopf_frequency(const Vec &poly) {
  const std::size_t n = poly.size() - 1;
  const auto eval = [&](std::complex<double> lam, std::complex<double> *deriv) {
    std::complex<double> v = poly[0], d = 0.0;
    for (std::size_t k = 1; k <= n; ++k) {
      d = d * lam + v;
      v = v * lam + poly[k];
    }
    if (deriv)
      *deriv = d;
    return v;
  };
  double scale = 0.0;
  for (std::size_t k = 1; k <= n; ++k)
    scale = std::max(scale, std::pow(std::abs(poly[k]), 1.0 / static_cast<double>(k)));
  if (scale == 0.0)
    return 0.0;
  double best = scale, bestv = std::numeric_limits<double>::infinity();
  for (int k = 0; k < 4000; ++k) {
    const double w = scale * std::pow(10.0, -4.0 + 8.0 * k / 3999.0);
    double mag = 0.0;
    for (std::size_t j = 0; j <= n; ++j)
      mag = std::max(mag, std::abs(poly[j]) * std::pow(w, static_cast<double>(n - j)));
    const double v = std::abs(eval({0.0, w}, nullptr)) / mag;
    if (v < bestv) {
      bestv = v;
      best = w;
    }
  }
  if (std::getenv("CONT_DEBUG")) { std::fprintf(stderr, "poly"); for (double c : poly) std::fprintf(stderr, " %g", c); std::fprintf(stderr, " grid best %g val %g\n", best, bestv); }
  std::complex<double> lam(0.0, best);
  for (int it = 0; it < 50; ++it) {
    std::complex<double> d;
    const auto v = eval(lam, &d);
    if (std::abs(d) == 0.0)
      break;
    const auto step = v / d;
    lam -= step;
    if (std::abs(step) < 1e-14 * std::abs(lam))
      break;
  }
  return std::abs(lam.imag());
}

} // namespace detail

// ---------------------------------------------------------------------------------------------------------------------------------
// Continuation of the branch through (u0, p0), which must satisfy G(u0, p0) ~ 0 (it is polished by the corrector).
// `direction` selects the sign of the initial p-velocity (+1 / -1); `initial_tangent` (size n + 1, in (u, p)) overrides it.
inline Branch continue_branch(const Residual &g, const Series &u0, double p0, const Options &opt = {}, double direction = 1.0,
                              const std::vector<double> &initial_tangent = {}) {
  using namespace detail;
  dynamics::require(u0.dim() == 1, "continuation: u0 must be a vector [n]");
  Context c;
  c.g = &g;
  c.n = u0.size(0);
  c.opt = opt;
  const auto n = static_cast<std::size_t>(c.n);
  c.w.assign(n + 1, 1.0);
  for (std::size_t i = 0; i < n && i < opt.u_scale.size(); ++i)
    c.w[i] = opt.u_scale[i];
  Vec z = u0.to(pluss::ScalarType::Float64).contiguous().host<double>().array().data;
  z.push_back(p0);

  // starting point: polish with the corrector, tangent from the sign of `direction` (or the supplied tangent)
  Vec t;
  {
    Lin l = linearize(c, z);
    Vec ref(n + 1, 0.0);
    if (!initial_tangent.empty()) {
      ref = initial_tangent;
    } else {
      ref[n] = direction >= 0 ? 1.0 : -1.0;
    }
    t = tangent(c, l, ref);
    if (!initial_tangent.empty()) {
      t = initial_tangent;
      const double nt = c.norm(t);
      for (auto &v : t)
        v /= nt;
    } else if (t[n] * ref[n] < 0.0) {
      for (auto &v : t)
        v = -v;
    }
    const auto first = correct(c, z, t);
    dynamics::require(first.ok, "continuation: the starting point does not converge to a solution branch");
    z = first.z;
  }

  Branch out;
  Vec us;
  std::vector<double> ps, unst;
  const auto record = [&](const Vec &zz, int count) {
    us.insert(us.end(), zz.begin(), zz.begin() + static_cast<std::ptrdiff_t>(n));
    ps.push_back(zz[n]);
    unst.push_back(static_cast<double>(count));
  };

  Corrected cur;
  cur.z = z;
  cur.lin = linearize(c, z);
  cur.t = t;
  {
    Vec d;
    cur.det = lu_solve(bordered(cur.lin, t, n), Vec(n + 1, 0.0), n + 1, d, false);
  }
  Spectrum spec_cur;
  if (opt.stability)
    spec_cur = spectrum(c, cur.lin);
  record(cur.z, spec_cur.unstable);
  double ds = opt.step;
  std::int64_t index = 1;

  const auto refine = [&](const Corrected &a, const std::function<bool(const Corrected &)> &predicate, double ds_trial,
                          Corrected &found) {
    // bisection on the arclength fraction s in (0, 1]: predicate false at 0 (point a), true at 1 (the accepted step);
    double lo = 0.0, hi = 1.0;
    Corrected best;
    for (int it = 0; it < 60 && hi - lo > 1e-13; ++it) {
      const double mid = 0.5 * (lo + hi);
      Vec zp(n + 1);
      for (std::size_t i = 0; i <= n; ++i)
        zp[i] = a.z[i] + mid * ds_trial * a.t[i];
      const auto trial = correct(c, zp, a.t);
      if (!trial.ok) {
        hi = mid;
        continue;
      }
      if (predicate(trial)) {
        hi = mid;
        best = trial;
      } else {
        lo = mid;
      }
    }
    if (!best.ok)
      return false;
    found = best;
    return true;
  };

  while (static_cast<std::int64_t>(ps.size()) < opt.max_points) {
    // predictor + corrector with step control
    Corrected next;
    bool accepted = false;
    while (ds >= opt.step_min) {
      Vec zp(n + 1);
      for (std::size_t i = 0; i <= n; ++i)
        zp[i] = cur.z[i] + ds * cur.t[i];
      next = correct(c, zp, cur.t);
      if (next.ok && c.wdot(next.t, cur.t) > 0.5) {
        accepted = true;
        break;
      }
      ds *= 0.5;
    }
    if (!accepted) {
      out.stopped = "step too small";
      break;
    }
    Spectrum spec_next;
    if (opt.stability)
      spec_next = spectrum(c, next.lin);

    if (opt.detect) {
      const double tp_a = cur.t[n], tp_b = next.t[n];
      if (tp_a * tp_b < 0.0) {
        Corrected f;
        const double sa = tp_a > 0 ? 1.0 : -1.0;
        if (refine(cur, [&](const Corrected &x) { return x.t[n] * sa <= 0.0; }, ds, f))
          out.special.push_back({PointKind::Fold, index, f.z[n], sub_u(f.z, n), 0.0});
      }
      if (cur.det * next.det < 0.0) {
        Corrected f;
        const double sa = cur.det > 0 ? 1.0 : -1.0;
        if (refine(cur, [&](const Corrected &x) { return x.det * sa <= 0.0; }, ds, f))
          out.special.push_back({PointKind::BranchPoint, index, f.z[n], sub_u(f.z, n), 0.0});
      }
      if (opt.stability && std::abs(spec_next.unstable - spec_cur.unstable) == 2 && spec_cur.det_gu * spec_next.det_gu > 0.0) {
        Corrected f;
        const int before = spec_cur.unstable;
        const bool grows = spec_next.unstable > before;
        if (refine(cur,
                   [&](const Corrected &x) {
                     const int k = spectrum(c, x.lin).unstable;
                     return grows ? k > before : k < before;
                   },
                   ds, f)) {
          const auto sp = spectrum(c, f.lin);
          out.special.push_back({PointKind::Hopf, index, f.z[n], sub_u(f.z, n), detail::hopf_frequency(sp.poly)});
        }
      }
    }
    cur = next;
    spec_cur = spec_next;
    record(cur.z, spec_cur.unstable);
    ++index;
    // step adaptation: fast convergence -> grow, slow -> shrink
    if (next.iterations <= 2)
      ds = std::min(opt.step_max, ds * 1.6);
    else if (next.iterations >= 5)
      ds = std::max(opt.step_min, ds * 0.6);
    if (cur.z[n] < opt.p_min || cur.z[n] > opt.p_max) {
      out.stopped = "p_min/p_max reached";
      break;
    }
  }
  if (out.stopped.empty())
    out.stopped = "max_points";
  const auto N = static_cast<std::int64_t>(ps.size());
  out.points = N;
  out.u = column(us, N, c.n);
  out.p = pluss::make_series(ps, {N}).to(pluss::ScalarType::Float64);
  out.unstable = pluss::make_series(unst, {N}).to(pluss::ScalarType::Float64);
  return out;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Branch switching. At a branch point z0 = (u0, p0) of G = 0 the Jacobian [G_u G_p] has a two-dimensional null space. Returns the
// tangent directions (in (u, p), unit length) of every branch through the point: the known branch (along `t_known`) and the others,
// from the algebraic bifurcation equation c11 a^2 + 2 c12 a b + c22 b^2 = 0 with
//   c_kl = psi^T G_zz[phi_k, phi_l],   psi = left null vector of [G_u G_p], phi_1 = t_known, phi_2 = null vector orthogonal to it.
// Returns an empty vector when the point is not a simple branch point (rank defect != 1 or degenerate quadratic form).
inline std::vector<std::vector<double>> branch_directions(const Residual &g, const Series &u0, double p0, const std::vector<double> &t_known,
                                                          double fd_step = 1e-4) {
  using namespace detail;
  Context c;
  c.g = &g;
  c.n = u0.size(0);
  c.opt.fd_step = 1e-6;
  const auto n = static_cast<std::size_t>(c.n);
  c.w.assign(n + 1, 1.0);
  Vec z = u0.to(pluss::ScalarType::Float64).contiguous().host<double>().array().data;
  z.push_back(p0);
  const Lin l = linearize(c, z);
  // null space of J (n x (n+1)): t_known is one vector; the second is the null vector of the bordered system with t_known as border,
  // obtained by solving [J; t_known^T] x = e for a bordered-singular matrix -> use the SVD-free approach: Gram-Schmidt on the
  // least-squares null vector via inverse iteration on J^T J + t t^T.
  Vec t1 = t_known;
  {
    const double nt = std::sqrt(dot(t1, t1));
    for (auto &v : t1)
      v /= nt;
  }
  // inverse iteration for the null vector of J orthogonal to t1: M = J^T J + (t1 t1^T) * big
  const std::size_t m = n + 1;
  Vec M(m * m, 0.0);
  for (std::size_t i = 0; i < m; ++i)
    for (std::size_t j = 0; j < m; ++j) {
      double s = 0.0;
      for (std::size_t k = 0; k < n; ++k)
        s += l.jac[k * m + i] * l.jac[k * m + j];
      M[i * m + j] = s + 1e6 * t1[i] * t1[j];
    }
  for (std::size_t i = 0; i < m; ++i)
    M[i * m + i] += 1e-13;
  Vec phi2(m, 0.0);
  for (std::size_t i = 0; i < m; ++i)
    phi2[i] = std::sin(1.0 + 1.7 * static_cast<double>(i)) + 0.1;
  for (int it = 0; it < 8; ++it) {
    Vec x;
    lu_solve(M, phi2, m, x);
    const double nx = std::sqrt(dot(x, x));
    if (!(nx > 0.0) || !std::isfinite(nx))
      return {};
    for (std::size_t i = 0; i < m; ++i)
      phi2[i] = x[i] / nx;
  }
  {
    const double proj = dot(phi2, t1);
    for (std::size_t i = 0; i < m; ++i)
      phi2[i] -= proj * t1[i];
    const double nn = std::sqrt(dot(phi2, phi2));
    for (auto &v : phi2)
      v /= nn;
  }
  // residual of J phi2 must vanish (rank defect exactly 2 at a simple branch point)
  {
    double r = 0.0;
    for (std::size_t k = 0; k < n; ++k) {
      double s = 0.0;
      for (std::size_t j = 0; j < m; ++j)
        s += l.jac[k * m + j] * phi2[j];
      r = std::max(r, std::abs(s));
    }
    if (r > 1e-4) {
      if (std::getenv("CONT_DEBUG")) std::fprintf(stderr, "null residual %g\n", r);
      return {};
    }
  }
  // left null vector psi of J (n x (n+1)): J J^T psi = small -> inverse iteration
  Vec JJt(n * n, 0.0);
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) {
      double s = 0.0;
      for (std::size_t k = 0; k < m; ++k)
        s += l.jac[i * m + k] * l.jac[j * m + k];
      JJt[i * n + j] = s;
    }
  Vec psi(n);
  for (std::size_t i = 0; i < n; ++i)
    psi[i] = std::cos(0.3 + 1.3 * static_cast<double>(i)) + 0.2;
  for (std::size_t i = 0; i < n; ++i)
    JJt[i * n + i] += 1e-14;
  for (int it = 0; it < 8; ++it) {
    Vec x;
    lu_solve(JJt, psi, n, x);
    const double nx = std::sqrt(dot(x, x));
    if (!(nx > 0.0) || !std::isfinite(nx))
      return {};
    for (std::size_t i = 0; i < n; ++i)
      psi[i] = x[i] / nx;
  }
  // second directional derivatives G_zz[a, b] by central differences (one batched call, 9 lanes per pair combination)
  const auto second = [&](const Vec &a, const Vec &b) {
    // G_zz[a,b] ~ (G(z+ha+hb) - G(z+ha-hb) - G(z-ha+hb) + G(z-ha-hb)) / (4 h^2)
    const double h = fd_step;
    const std::size_t K = 4;
    Vec U(K * n), P(K);
    const double sa[4] = {1, 1, -1, -1}, sb[4] = {1, -1, 1, -1};
    for (std::size_t k = 0; k < K; ++k) {
      for (std::size_t i = 0; i < n; ++i)
        U[k * n + i] = z[i] + h * (sa[k] * a[i] + sb[k] * b[i]);
      P[k] = z[n] + h * (sa[k] * a[n] + sb[k] * b[n]);
    }
    const Vec f = (*c.g)(column(U, static_cast<std::int64_t>(K), c.n), pluss::make_series(P, {static_cast<std::int64_t>(K)}).to(pluss::ScalarType::Float64))
                      .contiguous().host<double>().array().data;
    Vec out(n);
    for (std::size_t i = 0; i < n; ++i)
      out[i] = (f[0 * n + i] - f[1 * n + i] - f[2 * n + i] + f[3 * n + i]) / (4.0 * h * h);
    return out;
  };
  const double c11 = dot(psi, second(t1, t1)), c12 = dot(psi, second(t1, phi2)), c22 = dot(psi, second(phi2, phi2));
  // directions (alpha, beta) with c11 a^2 + 2 c12 a b + c22 b^2 = 0, a t1 + b phi2
  std::vector<std::pair<double, double>> sols;
  const double scale = std::abs(c11) + std::abs(c12) + std::abs(c22);
  if (std::getenv("CONT_DEBUG")) std::fprintf(stderr, "c11 %g c12 %g c22 %g\n", c11, c12, c22);
  if (scale < 1e-12)
    return {};
  // solve in terms of ratio r = b / a (and the case a = 0)
  const double disc = c12 * c12 - c11 * c22;
  if (disc < -1e-9 * scale * scale)
    return {}; // no real branches (isolated point)
  const double sq = std::sqrt(std::max(0.0, disc));
  if (std::abs(c22) > 1e-12 * scale) {
    sols.push_back({1.0, (-c12 + sq) / c22});
    sols.push_back({1.0, (-c12 - sq) / c22});
  } else {
    // c22 = 0: b = 0 solves it (the known branch direction a=1,b=0) or b/a = -c11/(2 c12)
    sols.push_back({1.0, 0.0});
    if (std::abs(c12) > 1e-12 * scale)
      sols.push_back({1.0, -c11 / (2.0 * c12)});
    sols.push_back({0.0, 1.0});
  }
  std::vector<std::vector<double>> dirs;
  for (const auto &s : sols) {
    Vec d(m);
    for (std::size_t i = 0; i < m; ++i)
      d[i] = s.first * t1[i] + s.second * phi2[i];
    const double nd = std::sqrt(dot(d, d));
    for (auto &v : d)
      v /= nd;
    bool dup = false;
    for (const auto &e : dirs) {
      double dp = 0.0;
      for (std::size_t i = 0; i < m; ++i)
        dp += d[i] * e[i];
      if (std::abs(std::abs(dp) - 1.0) < 1e-6)
        dup = true;
    }
    if (!dup)
      dirs.push_back(d);
  }
  return dirs;
}

} // namespace pluss::subsp::natsci::common::continuation
