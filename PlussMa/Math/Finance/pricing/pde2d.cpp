// The two-dimensional ADI solver declared in pde2d_internal.h.
#include "pde2d_internal.h"

#include "CodeM/core/Errors.h"
#include "pde_stencils_internal.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace pluss::subsp::math::finance::internal::pde2d {
namespace {

using Vec = std::vector<double>;

// Builds one line operator entry for the node `k` of an axis with `n` nodes, spacing from `g`, coefficients (second, drift, discount).
// `low`/`high` are the edge kinds.  Returns the row as (lo, mid, hi, ext) where ext multiplies the third node from the end the row sits on.
struct Row {
  double lo = 0.0, mid = 0.0, hi = 0.0, ext = 0.0;
};

Row make_row(const Vec& g, std::size_t k, double second, double drift, double discount, Edge low, Edge high) {
  const std::size_t n = g.size();
  Row r;
  r.mid = -0.5 * discount;
  if (k == 0) {
    if (low == Edge::Degenerate) {
      const auto f = internal::pde::first_forward(g[1] - g[0], g[2] - g[1]);
      r.mid += drift * f.lo;
      r.hi = drift * f.mid;
      r.ext = drift * f.hi;
    }
    return r;
  }
  if (k + 1 == n) {
    if (high == Edge::Degenerate) {
      const auto b = internal::pde::last_backward(g[n - 1] - g[n - 2], g[n - 2] - g[n - 3]);
      r.lo = drift * b.mid;
      r.mid += drift * b.hi;
      r.ext = drift * b.lo;
    }
    return r;
  }
  const double hm = g[k] - g[k - 1], hp = g[k + 1] - g[k];
  const auto d1 = internal::pde::first_central(hm, hp), d2 = internal::pde::second_central(hm, hp);
  r.lo = 0.5 * second * d2.lo + drift * d1.lo;
  r.mid += 0.5 * second * d2.mid + drift * d1.mid;
  r.hi = 0.5 * second * d2.hi + drift * d1.hi;
  return r;
}

void lagrange4(const Vec& g, double at, std::size_t& first, double w[4]) {
  std::size_t hi = static_cast<std::size_t>(std::upper_bound(g.begin(), g.end(), at) - g.begin());
  hi = std::clamp<std::size_t>(hi, 2, g.size() - 2);
  first = hi - 2;
  for (int a = 0; a < 4; ++a) {
    double p = 1.0;
    for (int b = 0; b < 4; ++b)
      if (b != a) p *= (at - g[first + static_cast<std::size_t>(b)]) / (g[first + static_cast<std::size_t>(a)] - g[first + static_cast<std::size_t>(b)]);
    w[a] = p;
  }
}

}  // namespace

std::vector<double> clustered_grid(double lo, double hi, double centre, double width, std::size_t n) {
  PLUSS_CHECK(n >= 4 && lo < centre && centre < hi && width > 0.0, "subsp::math::finance::pde2d::clustered_grid: need n >= 4, lo < centre < hi and width > 0");
  const double xi_lo = std::asinh((lo - centre) / width), xi_hi = std::asinh((hi - centre) / width);
  const double step0 = (xi_hi - xi_lo) / static_cast<double>(n - 1);
  const double below = std::max(1.0, std::round(-xi_lo / step0));  // make xi = 0 a node: the centre is where a payoff has its kink
  const double step = -xi_lo / below;
  Vec g(n);
  for (std::size_t i = 0; i < n; ++i) g[i] = centre + width * std::sinh(xi_lo + step * static_cast<double>(i));
  g[0] = lo;
  return g;
}

std::vector<double> origin_grid(double hi, double scale, std::size_t n) {
  PLUSS_CHECK(n >= 4 && hi > 0.0 && scale > 0.0, "subsp::math::finance::pde2d::origin_grid: need n >= 4, hi > 0 and scale > 0");
  const double eta = std::asinh(hi / scale);
  Vec g(n);
  for (std::size_t i = 0; i < n; ++i) g[i] = scale * std::sinh(eta * static_cast<double>(i) / static_cast<double>(n - 1));
  g[0] = 0.0;
  return g;
}

Solver::Solver(Problem problem) : p_(std::move(problem)) {
  nx_ = p_.x.size();
  ny_ = p_.y.size();
  PLUSS_CHECK(nx_ >= 4 && ny_ >= 4, "subsp::math::finance::pde2d: each grid needs at least four nodes");
  PLUSS_CHECK(static_cast<bool>(p_.generator) && static_cast<bool>(p_.payoff), "subsp::math::finance::pde2d: generator and payoff are required");
  PLUSS_CHECK(std::is_sorted(p_.x.begin(), p_.x.end()) && std::is_sorted(p_.y.begin(), p_.y.end()), "subsp::math::finance::pde2d: grids must be ascending");
  const auto need = [&](Edge e, const auto& fn, const char* name) {
    PLUSS_CHECK(e != Edge::Dirichlet || static_cast<bool>(fn), std::string("subsp::math::finance::pde2d: the Dirichlet edge ") + name + " needs a value function");
  };
  need(p_.x_low, p_.x_low_value, "x_low");
  need(p_.x_high, p_.x_high_value, "x_high");
  need(p_.y_low, p_.y_low_value, "y_low");
  need(p_.y_high, p_.y_high_value, "y_high");
  n_ = nx_ * ny_;
  build_();
}

void Solver::build_() {
  for (auto* l : {&ax_, &ay_}) {
    l->lo.assign(n_, 0.0);
    l->mid.assign(n_, 0.0);
    l->hi.assign(n_, 0.0);
    l->ext.assign(n_, 0.0);
  }
  const bool tangent = static_cast<bool>(p_.generator_tangent);
  if (tangent)
    for (auto* l : {&dax_, &day_}) {
      l->lo.assign(n_, 0.0);
      l->mid.assign(n_, 0.0);
      l->hi.assign(n_, 0.0);
      l->ext.assign(n_, 0.0);
    }
  cross_.assign(n_, 0.0);
  payoff_.resize(n_);
  const double tiny = 1e-14;
  for (std::size_t j = 0; j < ny_; ++j) {
    for (std::size_t i = 0; i < nx_; ++i) {
      const std::size_t k = index(i, j);
      const Coefficients co = p_.generator(p_.x[i], p_.y[j]);
      payoff_[k] = p_.payoff(p_.x[i], p_.y[j]);
      if (i == 0 && p_.x_low == Edge::Degenerate) PLUSS_CHECK(co.b1 >= -tiny, "subsp::math::finance::pde2d: the drift b1 points out of the domain on the Degenerate edge x_low; impose a Dirichlet value instead");
      if (i + 1 == nx_ && p_.x_high == Edge::Degenerate) PLUSS_CHECK(co.b1 <= tiny, "subsp::math::finance::pde2d: the drift b1 points out of the domain on the Degenerate edge x_high; impose a Dirichlet value instead");
      if (j == 0 && p_.y_low == Edge::Degenerate) PLUSS_CHECK(co.b2 >= -tiny, "subsp::math::finance::pde2d: the drift b2 points out of the domain on the Degenerate edge y_low; impose a Dirichlet value instead");
      if (j + 1 == ny_ && p_.y_high == Edge::Degenerate) PLUSS_CHECK(co.b2 <= tiny, "subsp::math::finance::pde2d: the drift b2 points out of the domain on the Degenerate edge y_high; impose a Dirichlet value instead");
      const Row rx = make_row(p_.x, i, co.a11, co.b1, co.c, p_.x_low, p_.x_high);
      const Row ry = make_row(p_.y, j, co.a22, co.b2, co.c, p_.y_low, p_.y_high);
      ax_.lo[k] = rx.lo; ax_.mid[k] = rx.mid; ax_.hi[k] = rx.hi; ax_.ext[k] = rx.ext;
      ay_.lo[k] = ry.lo; ay_.mid[k] = ry.mid; ay_.hi[k] = ry.hi; ay_.ext[k] = ry.ext;
      if (tangent) {
        const Coefficients dc = p_.generator_tangent(p_.x[i], p_.y[j]);
        const Row dx = make_row(p_.x, i, dc.a11, dc.b1, dc.c, p_.x_low, p_.x_high), dy = make_row(p_.y, j, dc.a22, dc.b2, dc.c, p_.y_low, p_.y_high);
        dax_.lo[k] = dx.lo; dax_.mid[k] = dx.mid; dax_.hi[k] = dx.hi; dax_.ext[k] = dx.ext;
        day_.lo[k] = dy.lo; day_.mid[k] = dy.mid; day_.hi[k] = dy.hi; day_.ext[k] = dy.ext;
      }
      if (i > 0 && i + 1 < nx_ && j > 0 && j + 1 < ny_) cross_[k] = co.a12;
    }
  }
  auto central = [](const Vec& g, Vec& lo, Vec& mid, Vec& hi) {
    lo.assign(g.size(), 0.0);
    mid.assign(g.size(), 0.0);
    hi.assign(g.size(), 0.0);
    for (std::size_t i = 1; i + 1 < g.size(); ++i) {
      const auto w = internal::pde::first_central(g[i] - g[i - 1], g[i + 1] - g[i]);
      lo[i] = w.lo;
      mid[i] = w.mid;
      hi[i] = w.hi;
    }
  };
  central(p_.x, wx_lo_, wx_mid_, wx_hi_);
  central(p_.y, wy_lo_, wy_mid_, wy_hi_);
  for (auto* v : {&a0u_, &a1u_, &a2u_, &a0y_, &a1y_, &a2y_, &y0_, &y1_, &rhs_}) v->assign(n_, 0.0);
  const std::size_t longest = std::max(nx_, ny_);
  for (auto* v : {&sub_, &diag_, &sup_, &scratch_, &line_}) v->assign(longest, 0.0);
  bc_x_low_.assign(ny_, 0.0);
  bc_x_high_.assign(ny_, 0.0);
  bc_y_low_.assign(nx_, 0.0);
  bc_y_high_.assign(nx_, 0.0);
}

void Solver::set_boundary_values_(double tau) {
  if (p_.x_low == Edge::Dirichlet) for (std::size_t j = 0; j < ny_; ++j) bc_x_low_[j] = p_.x_low_value(tau, p_.y[j]);
  if (p_.x_high == Edge::Dirichlet) for (std::size_t j = 0; j < ny_; ++j) bc_x_high_[j] = p_.x_high_value(tau, p_.y[j]);
  if (p_.y_low == Edge::Dirichlet) for (std::size_t i = 0; i < nx_; ++i) bc_y_low_[i] = p_.y_low_value(tau, p_.x[i]);
  if (p_.y_high == Edge::Dirichlet) for (std::size_t i = 0; i < nx_; ++i) bc_y_high_[i] = p_.y_high_value(tau, p_.x[i]);
}

void Solver::apply_mixed_(const Vec& u, Vec& out) const {
  std::fill(out.begin(), out.end(), 0.0);
  for (std::size_t j = 1; j + 1 < ny_; ++j) {
    const double* r0 = u.data() + (j - 1) * nx_;
    const double* r1 = r0 + nx_;
    const double* r2 = r1 + nx_;
    const double v0 = wy_lo_[j], v1 = wy_mid_[j], v2 = wy_hi_[j];
    double* o = out.data() + j * nx_;
    const double* cr = cross_.data() + j * nx_;
    for (std::size_t i = 1; i + 1 < nx_; ++i) {
      const double s0 = wx_lo_[i], s1 = wx_mid_[i], s2 = wx_hi_[i];
      const double a0 = s0 * r0[i - 1] + s1 * r0[i] + s2 * r0[i + 1];
      const double a1 = s0 * r1[i - 1] + s1 * r1[i] + s2 * r1[i + 1];
      const double a2 = s0 * r2[i - 1] + s1 * r2[i] + s2 * r2[i + 1];
      o[i] = cr[i] * (v0 * a0 + v1 * a1 + v2 * a2);
    }
  }
}

void Solver::apply_x_(const Vec& u, Vec& out) const { apply_x_(ax_, u, out); }
void Solver::apply_y_(const Vec& u, Vec& out) const { apply_y_(ay_, u, out); }

void Solver::apply_x_(const Lines& ax_, const Vec& u, Vec& out) const {
  for (std::size_t j = 0; j < ny_; ++j)
    for (std::size_t i = 0; i < nx_; ++i) {
      const std::size_t k = index(i, j);
      double acc = ax_.mid[k] * u[k];
      if (i > 0) acc += ax_.lo[k] * u[k - 1];
      if (i + 1 < nx_) acc += ax_.hi[k] * u[k + 1];
      if (i == 0) acc += ax_.ext[k] * u[k + 2];
      if (i + 1 == nx_) acc += ax_.ext[k] * u[k - 2];
      out[k] = acc;
    }
}

void Solver::apply_y_(const Lines& ay_, const Vec& u, Vec& out) const {
  for (std::size_t j = 0; j < ny_; ++j)
    for (std::size_t i = 0; i < nx_; ++i) {
      const std::size_t k = index(i, j);
      double acc = ay_.mid[k] * u[k];
      if (j > 0) acc += ay_.lo[k] * u[index(i, j - 1)];
      if (j + 1 < ny_) acc += ay_.hi[k] * u[index(i, j + 1)];
      if (j == 0) acc += ay_.ext[k] * u[index(i, 2)];
      if (j + 1 == ny_) acc += ay_.ext[k] * u[index(i, j - 2)];
      out[k] = acc;
    }
}

// The factorization of (I - c A) along one axis, line by line: the elimination of the Thomas algorithm, kept.  A Dirichlet edge is an identity row;
// a row with a third entry (a Degenerate edge) is first reduced to tridiagonal form by one elimination step against its neighbour, whose factor is
// stored in `first` / `last`.  A line lying on a Dirichlet edge of the other axis is an identity line.
const Solver::Factor& Solver::factor_x_(double c) {
  auto found = factors_x_.find(c);
  if (found != factors_x_.end()) return found->second;
  Factor f;
  f.sub.assign(n_, 0.0); f.inv.assign(n_, 1.0); f.sc.assign(n_, 0.0);
  f.first.assign(ny_, 0.0); f.last.assign(ny_, 0.0);
  const std::size_t n = nx_, last = nx_ - 1;
  Vec sub(n), diag(n), sup(n);
  for (std::size_t j = 0; j < ny_; ++j) {
    if ((j == 0 && p_.y_low == Edge::Dirichlet) || (j + 1 == ny_ && p_.y_high == Edge::Dirichlet)) continue;  // identity line
    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t k = index(i, j);
      sub[i] = -c * ax_.lo[k];
      diag[i] = 1.0 - c * ax_.mid[k];
      sup[i] = -c * ax_.hi[k];
    }
    const std::size_t k0 = index(0, j), kn = index(last, j);
    if (p_.x_low == Edge::Dirichlet) {
      sub[0] = 0.0; diag[0] = 1.0; sup[0] = 0.0;
    } else if (ax_.ext[k0] != 0.0) {
      f.first[j] = (-c * ax_.ext[k0]) / sup[1];
      diag[0] -= f.first[j] * sub[1];
      sup[0] -= f.first[j] * diag[1];
    }
    if (p_.x_high == Edge::Dirichlet) {
      sub[last] = 0.0; diag[last] = 1.0; sup[last] = 0.0;
    } else if (ax_.ext[kn] != 0.0) {
      f.last[j] = (-c * ax_.ext[kn]) / sub[last - 1];
      sub[last] -= f.last[j] * diag[last - 1];
      diag[last] -= f.last[j] * sup[last - 1];
    }
    double prev = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      const double pivot = diag[i] - sub[i] * prev;
      f.inv[index(i, j)] = 1.0 / pivot;
      prev = sup[i] / pivot;
      f.sc[index(i, j)] = prev;
      f.sub[index(i, j)] = sub[i];
    }
  }
  return factors_x_.emplace(c, std::move(f)).first->second;
}

const Solver::Factor& Solver::factor_y_(double c) {
  auto found = factors_y_.find(c);
  if (found != factors_y_.end()) return found->second;
  Factor f;
  f.sub.assign(n_, 0.0); f.inv.assign(n_, 1.0); f.sc.assign(n_, 0.0);
  f.first.assign(nx_, 0.0); f.last.assign(nx_, 0.0);
  const std::size_t n = ny_, last = ny_ - 1;
  Vec sub(n), diag(n), sup(n);
  for (std::size_t i = 0; i < nx_; ++i) {
    if ((i == 0 && p_.x_low == Edge::Dirichlet) || (i + 1 == nx_ && p_.x_high == Edge::Dirichlet)) continue;  // identity line
    for (std::size_t j = 0; j < n; ++j) {
      const std::size_t k = index(i, j);
      sub[j] = -c * ay_.lo[k];
      diag[j] = 1.0 - c * ay_.mid[k];
      sup[j] = -c * ay_.hi[k];
    }
    const std::size_t k0 = index(i, 0), kn = index(i, last);
    if (p_.y_low == Edge::Dirichlet) {
      sub[0] = 0.0; diag[0] = 1.0; sup[0] = 0.0;
    } else if (ay_.ext[k0] != 0.0) {
      f.first[i] = (-c * ay_.ext[k0]) / sup[1];
      diag[0] -= f.first[i] * sub[1];
      sup[0] -= f.first[i] * diag[1];
    }
    if (p_.y_high == Edge::Dirichlet) {
      sub[last] = 0.0; diag[last] = 1.0; sup[last] = 0.0;
    } else if (ay_.ext[kn] != 0.0) {
      f.last[i] = (-c * ay_.ext[kn]) / sub[last - 1];
      sub[last] -= f.last[i] * diag[last - 1];
      diag[last] -= f.last[i] * sup[last - 1];
    }
    double prev = 0.0;
    for (std::size_t j = 0; j < n; ++j) {
      const double pivot = diag[j] - sub[j] * prev;
      f.inv[index(i, j)] = 1.0 / pivot;
      prev = sup[j] / pivot;
      f.sc[index(i, j)] = prev;
      f.sub[index(i, j)] = sub[j];
    }
  }
  return factors_y_.emplace(c, std::move(f)).first->second;
}

// Solves (I - c A_x) x = rhs in place along every line of constant y (contiguous in memory).  Each line is a serial recurrence, so four lines are
// swept together: four independent chains keep the processor busy where one would wait on its own previous result.
void Solver::solve_x_(Vec& rhs, double c) {
  const Factor& f = factor_x_(c);
  const std::size_t n = nx_, last = nx_ - 1;
  for (std::size_t j = 0; j < ny_; ++j) {  // boundary values and end-row eliminations; the identity factors of a Dirichlet line leave it as set
    double* r = rhs.data() + j * n;
    if (j == 0 && p_.y_low == Edge::Dirichlet) { for (std::size_t i = 0; i < n; ++i) r[i] = bc_y_low_[i]; continue; }
    if (j + 1 == ny_ && p_.y_high == Edge::Dirichlet) { for (std::size_t i = 0; i < n; ++i) r[i] = bc_y_high_[i]; continue; }
    if (p_.x_low == Edge::Dirichlet) r[0] = bc_x_low_[j];
    else if (f.first[j] != 0.0) r[0] -= f.first[j] * r[1];
    if (p_.x_high == Edge::Dirichlet) r[last] = bc_x_high_[j];
    else if (f.last[j] != 0.0) r[last] -= f.last[j] * r[last - 1];
  }
  std::size_t j = 0;
  for (; j + 4 <= ny_; j += 4) {
    double* r0 = rhs.data() + j * n; double* r1 = r0 + n; double* r2 = r1 + n; double* r3 = r2 + n;
    const double *s0 = f.sub.data() + j * n, *s1 = s0 + n, *s2 = s1 + n, *s3 = s2 + n;
    const double *v0 = f.inv.data() + j * n, *v1 = v0 + n, *v2 = v1 + n, *v3 = v2 + n;
    const double *c0 = f.sc.data() + j * n, *c1 = c0 + n, *c2 = c1 + n, *c3 = c2 + n;
    r0[0] *= v0[0]; r1[0] *= v1[0]; r2[0] *= v2[0]; r3[0] *= v3[0];
    for (std::size_t i = 1; i < n; ++i) {
      r0[i] = (r0[i] - s0[i] * r0[i - 1]) * v0[i];
      r1[i] = (r1[i] - s1[i] * r1[i - 1]) * v1[i];
      r2[i] = (r2[i] - s2[i] * r2[i - 1]) * v2[i];
      r3[i] = (r3[i] - s3[i] * r3[i - 1]) * v3[i];
    }
    for (std::size_t i = n - 1; i-- > 0;) {
      r0[i] -= c0[i] * r0[i + 1];
      r1[i] -= c1[i] * r1[i + 1];
      r2[i] -= c2[i] * r2[i + 1];
      r3[i] -= c3[i] * r3[i + 1];
    }
  }
  for (; j < ny_; ++j) {
    double* r = rhs.data() + j * n;
    const double *sub = f.sub.data() + j * n, *inv = f.inv.data() + j * n, *sc = f.sc.data() + j * n;
    r[0] *= inv[0];
    for (std::size_t i = 1; i < n; ++i) r[i] = (r[i] - sub[i] * r[i - 1]) * inv[i];
    for (std::size_t i = n - 1; i-- > 0;) r[i] -= sc[i] * r[i + 1];
  }
}

// Solves (I - c A_y) x = rhs in place along every line of constant x.  The lines are swept together: for each j the inner loop runs over x with
// unit stride.
void Solver::solve_y_(Vec& rhs, double c) {
  const Factor& f = factor_y_(c);
  const std::size_t nx = nx_, ny = ny_, last = ny_ - 1;
  for (std::size_t i = 0; i < nx; ++i) {
    if (i == 0 && p_.x_low == Edge::Dirichlet) { for (std::size_t j = 0; j < ny; ++j) rhs[index(i, j)] = bc_x_low_[j]; continue; }
    if (i + 1 == nx && p_.x_high == Edge::Dirichlet) { for (std::size_t j = 0; j < ny; ++j) rhs[index(i, j)] = bc_x_high_[j]; continue; }
    if (p_.y_low == Edge::Dirichlet) rhs[index(i, 0)] = bc_y_low_[i];
    else if (f.first[i] != 0.0) rhs[index(i, 0)] -= f.first[i] * rhs[index(i, 1)];
    if (p_.y_high == Edge::Dirichlet) rhs[index(i, last)] = bc_y_high_[i];
    else if (f.last[i] != 0.0) rhs[index(i, last)] -= f.last[i] * rhs[index(i, last - 1)];
  }
  double* r = rhs.data();
  for (std::size_t i = 0; i < nx; ++i) r[i] *= f.inv[i];
  for (std::size_t j = 1; j < ny; ++j) {
    double* cur = r + j * nx;
    const double* prev = cur - nx;
    const double* sub = f.sub.data() + j * nx;
    const double* inv = f.inv.data() + j * nx;
    for (std::size_t i = 0; i < nx; ++i) cur[i] = (cur[i] - sub[i] * prev[i]) * inv[i];
  }
  for (std::size_t j = ny - 1; j-- > 0;) {
    double* cur = r + j * nx;
    const double* next = cur + nx;
    const double* sc = f.sc.data() + j * nx;
    for (std::size_t i = 0; i < nx; ++i) cur[i] -= sc[i] * next[i];
  }
}

// One step of length dt to time-to-maturity tau_new: Modified Craig-Sneyd (vartheta = 1/2: the Craig-Sneyd correction), or the Douglas scheme
// for the smoothing steps.  `lambda` is the lagged multiplier of the exercise splitting; for an American option the constraint follows.
void Solver::advance_(Vec& u, Vec& lambda, double dt, double tau_new, bool douglas, bool american, double theta) {
  set_boundary_values_(tau_new);
  apply_mixed_(u, a0u_);
  apply_x_(u, a1u_);
  apply_y_(u, a2u_);
  for (std::size_t k = 0; k < n_; ++k) y0_[k] = u[k] + dt * (a0u_[k] + a1u_[k] + a2u_[k] + (american ? lambda[k] : 0.0));
  if (douglas) {
    for (std::size_t k = 0; k < n_; ++k) rhs_[k] = y0_[k] - dt * a1u_[k];
    solve_x_(rhs_, dt);
    for (std::size_t k = 0; k < n_; ++k) rhs_[k] -= dt * a2u_[k];
    solve_y_(rhs_, dt);
    u.swap(rhs_);
  } else {
    constexpr double vartheta = 0.5;
    const double c = theta * dt;
    for (std::size_t k = 0; k < n_; ++k) rhs_[k] = y0_[k] - c * a1u_[k];
    solve_x_(rhs_, c);
    for (std::size_t k = 0; k < n_; ++k) rhs_[k] -= c * a2u_[k];
    solve_y_(rhs_, c);  // rhs_ = Y2
    y1_ = rhs_;
    apply_mixed_(y1_, a0y_);
    apply_x_(y1_, a1y_);
    apply_y_(y1_, a2y_);
    for (std::size_t k = 0; k < n_; ++k) {  // Y~0 = Y0 + vartheta dt (F(Y2) - F(U)); the (1/2 - vartheta) A0-correction vanishes
      y0_[k] += vartheta * dt * (a0y_[k] + a1y_[k] + a2y_[k] - a0u_[k] - a1u_[k] - a2u_[k]);
      rhs_[k] = y0_[k] - c * a1y_[k];
    }
    solve_x_(rhs_, c);
    for (std::size_t k = 0; k < n_; ++k) rhs_[k] -= c * a2y_[k];
    solve_y_(rhs_, c);
    u.swap(rhs_);
  }
  if (american) {  // Ikonen-Toivanen: remove the lagged multiplier, project onto the payoff, update the multiplier
    for (std::size_t k = 0; k < n_; ++k) {
      const double x = u[k] - dt * lambda[k];
      lambda[k] = std::max(0.0, (payoff_[k] - x) / dt);
      u[k] = std::max(x, payoff_[k]);
    }
  }
}

// advance_ with the tangent of the step in the parameter of Problem::generator_tangent. Every linear solve (I - c A) x = r differentiates to
// (I - c A) dx = dr + c dA x with the boundary rows set to the boundary tangent; the exercise projection differentiates along its active set.
void Solver::advance_tangent_(Vec& u, Vec& w, Vec& lambda, Vec& lambda_w, double dt, double tau_new, bool douglas, bool american, double theta) {
  set_boundary_values_(tau_new);
  const Vec primal_x_low = bc_x_low_, primal_x_high = bc_x_high_, primal_y_low = bc_y_low_, primal_y_high = bc_y_high_;
  Vec tangent_x_high(ny_, 0.0);
  if (p_.x_high == Edge::Dirichlet && p_.x_high_tangent) for (std::size_t j = 0; j < ny_; ++j) tangent_x_high[j] = p_.x_high_tangent(tau_new, p_.y[j]);
  const auto with_tangent_boundary = [&](auto&& body) {
    bc_x_low_.assign(ny_, 0.0); bc_x_high_ = tangent_x_high; bc_y_low_.assign(nx_, 0.0); bc_y_high_.assign(nx_, 0.0);
    body();
    bc_x_low_ = primal_x_low; bc_x_high_ = primal_x_high; bc_y_low_ = primal_y_low; bc_y_high_ = primal_y_high;
  };
  Vec tmp(n_);
  const auto solve_x_pair = [&](Vec& rhs, Vec& drhs, double c) {
    solve_x_(rhs, c);   // rhs now holds the primal solution
    apply_x_(dax_, rhs, tmp);
    for (std::size_t k = 0; k < n_; ++k) drhs[k] += c * tmp[k];
    with_tangent_boundary([&] { solve_x_(drhs, c); });
  };
  const auto solve_y_pair = [&](Vec& rhs, Vec& drhs, double c) {
    solve_y_(rhs, c);
    apply_y_(day_, rhs, tmp);
    for (std::size_t k = 0; k < n_; ++k) drhs[k] += c * tmp[k];
    with_tangent_boundary([&] { solve_y_(drhs, c); });
  };
  Vec a0u(n_), a1u(n_), a2u(n_), da0(n_), da1(n_), da2(n_), y0(n_), dy0(n_), rhs(n_), drhs(n_);
  apply_mixed_(u, a0u); apply_x_(u, a1u); apply_y_(u, a2u);
  apply_mixed_(w, da0); apply_x_(w, da1); apply_y_(w, da2);
  apply_x_(dax_, u, tmp); for (std::size_t k = 0; k < n_; ++k) da1[k] += tmp[k];
  apply_y_(day_, u, tmp); for (std::size_t k = 0; k < n_; ++k) da2[k] += tmp[k];
  for (std::size_t k = 0; k < n_; ++k) {
    y0[k] = u[k] + dt * (a0u[k] + a1u[k] + a2u[k] + (american ? lambda[k] : 0.0));
    dy0[k] = w[k] + dt * (da0[k] + da1[k] + da2[k] + (american ? lambda_w[k] : 0.0));
  }
  if (douglas) {
    for (std::size_t k = 0; k < n_; ++k) { rhs[k] = y0[k] - dt * a1u[k]; drhs[k] = dy0[k] - dt * da1[k]; }
    solve_x_pair(rhs, drhs, dt);
    for (std::size_t k = 0; k < n_; ++k) { rhs[k] -= dt * a2u[k]; drhs[k] -= dt * da2[k]; }
    solve_y_pair(rhs, drhs, dt);
  } else {
    constexpr double vartheta = 0.5;
    const double c = theta * dt;
    for (std::size_t k = 0; k < n_; ++k) { rhs[k] = y0[k] - c * a1u[k]; drhs[k] = dy0[k] - c * da1[k]; }
    solve_x_pair(rhs, drhs, c);
    for (std::size_t k = 0; k < n_; ++k) { rhs[k] -= c * a2u[k]; drhs[k] -= c * da2[k]; }
    solve_y_pair(rhs, drhs, c);   // Y2 and its tangent
    Vec a0y(n_), a1y(n_), a2y(n_), db0(n_), db1(n_), db2(n_);
    apply_mixed_(rhs, a0y); apply_x_(rhs, a1y); apply_y_(rhs, a2y);
    apply_mixed_(drhs, db0); apply_x_(drhs, db1); apply_y_(drhs, db2);
    apply_x_(dax_, rhs, tmp); for (std::size_t k = 0; k < n_; ++k) db1[k] += tmp[k];
    apply_y_(day_, rhs, tmp); for (std::size_t k = 0; k < n_; ++k) db2[k] += tmp[k];
    for (std::size_t k = 0; k < n_; ++k) {
      y0[k] += vartheta * dt * (a0y[k] + a1y[k] + a2y[k] - a0u[k] - a1u[k] - a2u[k]);
      dy0[k] += vartheta * dt * (db0[k] + db1[k] + db2[k] - da0[k] - da1[k] - da2[k]);
      rhs[k] = y0[k] - c * a1y[k];
      drhs[k] = dy0[k] - c * db1[k];
    }
    solve_x_pair(rhs, drhs, c);
    for (std::size_t k = 0; k < n_; ++k) { rhs[k] -= c * a2y[k]; drhs[k] -= c * db2[k]; }
    solve_y_pair(rhs, drhs, c);
  }
  u.swap(rhs);
  w.swap(drhs);
  if (american) {
    for (std::size_t k = 0; k < n_; ++k) {
      const double x = u[k] - dt * lambda[k], dx = w[k] - dt * lambda_w[k];
      if (payoff_[k] - x > 0.0) { lambda[k] = (payoff_[k] - x) / dt; lambda_w[k] = -dx / dt; u[k] = payoff_[k]; w[k] = 0.0; }
      else { lambda[k] = 0.0; lambda_w[k] = 0.0; u[k] = x; w[k] = dx; }
    }
  }
}

std::vector<double> Solver::solve(const Options& o, std::vector<double>* tangent) {
  PLUSS_CHECK(tangent == nullptr || static_cast<bool>(p_.generator_tangent), "subsp::math::finance::pde2d: a tangent needs Problem::generator_tangent");
  PLUSS_CHECK(o.steps >= 3 && p_.maturity > 0.0, "subsp::math::finance::pde2d: steps >= 3 and a positive maturity are required");
  const bool american = o.exercise == Exercise::American;
  Vec u = payoff_, lambda(n_, 0.0), w(n_, 0.0), lambda_w(n_, 0.0);
  const auto step = [&](double dt_, double tau_, bool douglas) {
    if (tangent) advance_tangent_(u, w, lambda, lambda_w, dt_, tau_, douglas, american, o.theta);
    else advance_(u, lambda, dt_, tau_, douglas, american, o.theta);
  };
  std::vector<double> edges{0.0};
  if (o.exercise == Exercise::Bermudan) edges.insert(edges.end(), o.breaks.begin(), o.breaks.end());
  edges.push_back(p_.maturity);
  double tau = 0.0;
  bool first = true;
  for (std::size_t seg = 0; seg + 1 < edges.size(); ++seg) {
    const double len = edges[seg + 1] - edges[seg];
    const auto count = std::max<std::int64_t>(first ? 3 : 2, static_cast<std::int64_t>(std::ceil(static_cast<double>(o.steps) * len / p_.maturity)));
    const double dt = len / static_cast<double>(count);
    std::int64_t done = 0;
    if (first) {  // Rannacher: two backward-Euler (Douglas, theta = 1) steps, each as two half steps, damp the payoff kink
      for (int h = 0; h < 4; ++h) {
        tau += 0.5 * dt;
        step(0.5 * dt, tau, true);
      }
      done = 2;
    }
    for (; done < count; ++done) {
      tau += dt;
      step(dt, tau, false);
    }
    first = false;
    if (o.exercise == Exercise::Bermudan && seg + 2 < edges.size())  // arrived at an exercise date
      for (std::size_t k = 0; k < n_; ++k) {
        if (payoff_[k] > u[k]) { u[k] = payoff_[k]; w[k] = 0.0; }
      }
  }
  if (tangent) *tangent = w;
  return u;
}

double Solver::interpolate(const std::vector<double>& u, double x0, double y0) const {
  std::size_t i0 = 0, j0 = 0;
  double wx[4], wy[4];
  lagrange4(p_.x, x0, i0, wx);
  lagrange4(p_.y, y0, j0, wy);
  double acc = 0.0;
  for (std::size_t b = 0; b < 4; ++b)
    for (std::size_t a = 0; a < 4; ++a) acc += wy[b] * wx[a] * u[index(i0 + a, j0 + b)];
  return acc;
}

}  // namespace pluss::subsp::math::finance::internal::pde2d
