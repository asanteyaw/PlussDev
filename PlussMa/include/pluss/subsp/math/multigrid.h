#pragma once

// Geometric multigrid for cell-centred, conservative, second-order operators on structured grids,
//
//     (A u)_i = - sum_a h_a^-2 [ b+_a,i (u_{i+e_a} - u_i) - b-_a,i (u_i - u_{i-e_a}) ]          (b = 1 where no coefficient is given)
//
// with per-axis periodic / Dirichlet / Neumann ends (homogeneous: the value beyond a wall is -u / +u). A is symmetric positive semi-definite
// (singular exactly when every axis is periodic or Neumann: the constant mode), so a V-cycle with identical Chebyshev pre/post smoothing and a
// restriction that is the adjoint of the prolongation is itself a symmetric positive operator: the right preconditioner for conjugate gradients.
//
//  * Smoother: Chebyshev polynomial on D^-1 A, targeting the upper part of the spectrum [hi / lo_ratio, hi] (hi = 2 by Gershgorin), no inner products,
//    so every level is a handful of Series expressions over the whole field and all batch lanes.
//  * Transfer: tensor-product linear interpolation with wall-reflected ghosts, restriction = adjoint / 2^d. Coarse operators are the rediscretisation
//    at spacing 2h: face coefficients are the coincident fine face, averaged over the 2^(d-1) transverse fine faces.
//  * Coarsening halves every axis while all extents are even and the level has more than `coarsest_cells` cells; the coarsest level is inverted
//    densely (pseudo-inverse, so singular problems are handled) when small enough, otherwise swept by a high-degree Chebyshev polynomial.
//  * Fields are [batch..., n_0, ..., n_{d-1}]; coefficient fields may carry the batch axes (one coefficient per lane) or be shared.
//  * Anisotropic spacings (h ratios far from 1) keep the preconditioner valid but lose the mesh-independent rate (no semi-coarsening).

#include "pluss/subsp/math/krylov.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace pluss::subsp::math::multigrid {

enum class Edge : std::uint8_t { Periodic = 0, Dirichlet = 1, Neumann = 2 };

struct Axis final {
  std::int64_t cells{};
  double spacing{1.0};
  Edge low{Edge::Periodic};
  Edge high{Edge::Periodic};
};

struct Problem final {
  std::vector<Axis> axes; // the last axes.size() dimensions of every field
  // Face coefficients per axis (empty = all ones): `face_high[a][..., i]` joins cell i to i+1 along axis a (positive, finite); `face_low[a][..., i]` joins
  // i-1 to i. At a wall the entry is the coefficient of the wall face. When only one of the pair is given the other is derived from it.
  std::vector<Series> face_low{};
  std::vector<Series> face_high{};
  // Optional non-negative mass term (A + diag(mass), cell-wise, in units of 1/length^2, averaged onto coarser levels). It removes the constant null mode,
  // which makes the V-cycle a positive definite preconditioner for operators that merely resemble A.
  Series mass = pluss::series::undefined();
};

struct Options final {
  std::int64_t smoothing_degree{3};
  std::int64_t coarsest_cells{64}; // stop coarsening at this many cells (per lane); the coarsest level is then inverted densely
  double smoothing_ratio{4.0};   // Chebyshev targets [2/ratio, 2.2]
  std::int64_t max_levels{32};
  std::int64_t dense_limit{1024}; // coarsest levels up to this many cells are inverted by a pseudo-inverse, larger ones smoothed
  bool native_smoother{true};     // CPU fields that record no derivative smooth with the fused native Chebyshev loop (identical polynomial)
  bool native_cycle{true};        // CPU fields that record no derivative run the whole V-cycle (smoothing, residual, transfers, coarse solve) on host arrays
};

namespace detail {

[[nodiscard]] inline double sign(Edge e) { return e == Edge::Dirichlet ? -1.0 : 1.0; }
[[nodiscard]] inline std::int64_t spatial_axis(const Series &x, std::size_t d, std::size_t a) { return x.dim() - static_cast<std::int64_t>(d) + static_cast<std::int64_t>(a); }
[[nodiscard]] inline Series cat(const Series &first, const Series &second, std::int64_t axis) { return first.unite({second}, axis); }

// neighbour fields along `axis` (lower / upper), wall-reflected where the axis is not periodic
[[nodiscard]] inline Series lower_neighbor(const Series &u, std::int64_t axis, const Axis &ax) {
  if (ax.low == Edge::Periodic)
    return u.roll(1, axis);
  const std::int64_t n = u.size(axis);
  if (n == 1)
    return u.mul(sign(ax.low));
  return cat(u.select(axis, 0).unsqueeze(axis).mul(sign(ax.low)), u.slice(axis, 0, n - 1), axis);
}
[[nodiscard]] inline Series upper_neighbor(const Series &u, std::int64_t axis, const Axis &ax) {
  if (ax.high == Edge::Periodic)
    return u.roll(-1, axis);
  const std::int64_t n = u.size(axis);
  if (n == 1)
    return u.mul(sign(ax.high));
  return cat(u.slice(axis, 1, n), u.select(axis, n - 1).unsqueeze(axis).mul(sign(ax.high)), axis);
}

[[nodiscard]] inline Series interleave(const Series &even, const Series &odd, std::int64_t axis) {
  std::vector<std::int64_t> shape(even.sizes().begin(), even.sizes().end());
  shape[static_cast<std::size_t>(axis)] *= 2;
  return cat(even.unsqueeze(axis + 1), odd.unsqueeze(axis + 1), axis + 1).reshape(shape);
}
[[nodiscard]] inline Series split_parity(const Series &x, std::int64_t axis, int parity) {
  std::vector<std::int64_t> shape(x.sizes().begin(), x.sizes().end());
  shape[static_cast<std::size_t>(axis)] /= 2;
  shape.insert(shape.begin() + axis + 1, 2);
  return x.reshape(shape).select(axis + 1, parity);
}
[[nodiscard]] inline Series pair_mean(const Series &x, std::int64_t axis) { return split_parity(x, axis, 0).add(split_parity(x, axis, 1)).mul(0.5); }

[[nodiscard]] inline Series axis_mask(std::int64_t n, std::int64_t position, double value, const Series &like, std::int64_t axis_in_field, std::int64_t rank) {
  std::vector<double> v(static_cast<std::size_t>(n), 0.0);
  v[static_cast<std::size_t>(position)] = value;
  std::vector<std::int64_t> shape(static_cast<std::size_t>(rank), 1);
  shape[static_cast<std::size_t>(axis_in_field)] = n;
  return pluss::series::make(v, shape).to(like.scalar_type()).to(like.platform());
}

// Interpolation weights (coarse-shaped): fine child 2I takes w_e of the lower coarse neighbour, child 2I+1 takes w_o of the upper one; undefined = 1/4.
// Both are ratios of series resistances along the axis, so the interpolant is flux-continuous across coefficient jumps (exact in 1-D).
[[nodiscard]] inline Series weighted(const Series &w, const Series &x, double constant) { return w.defined() ? x.mul(w) : x.mul(constant); }
[[nodiscard]] inline Series duplicate_fine(Series w, std::int64_t rank, std::size_t d, std::size_t upto) {
  if (!w.defined())
    return w;
  for (std::size_t b = 0; b < upto; ++b)
    w = interleave(w, w, rank - static_cast<std::int64_t>(d) + static_cast<std::int64_t>(b));
  return w;
}
[[nodiscard]] inline Series prolong_axis(const Series &c, std::int64_t axis, const Axis &ax, const Series &we, const Series &wo) {
  const Series below = lower_neighbor(c, axis, ax), above = upper_neighbor(c, axis, ax);
  const Series even = we.defined() ? c.sub(c.sub(below).mul(we)) : c.mul(0.75).add(below.mul(0.25));
  const Series odd = wo.defined() ? c.sub(c.sub(above).mul(wo)) : c.mul(0.75).add(above.mul(0.25));
  return interleave(even, odd, axis);
}
// adjoint of prolong_axis, times 1/2
[[nodiscard]] inline Series restrict_axis(const Series &f, std::int64_t axis, const Axis &ax, const Series &we, const Series &wo) {
  const Series fe = split_parity(f, axis, 0), fo = split_parity(f, axis, 1);
  const std::int64_t n = fe.size(axis);
  const Series x = weighted(we, fe, 0.25), y = weighted(wo, fo, 0.25); // x_I feeds c_{I-1}, y_I feeds c_{I+1}
  Series from_next, from_prev;
  if (ax.high == Edge::Periodic) {
    from_next = x.roll(-1, axis);
    from_prev = y.roll(1, axis);
  } else {
    // wall: the missing neighbour is the reflected ghost, c_{-1} = s c_0 (so x_0 returns to c_0) and c_n = s c_{n-1}
    from_next = n == 1 ? x.mul(0.0) : cat(x.slice(axis, 1, n), x.slice(axis, 0, 1).mul(0.0), axis);
    from_prev = n == 1 ? y.mul(0.0) : cat(y.slice(axis, 0, 1).mul(0.0), y.slice(axis, 0, n - 1), axis);
    from_prev = from_prev.add(axis_mask(n, 0, sign(ax.low), f, axis, f.dim()).mul(x));
    from_next = from_next.add(axis_mask(n, n - 1, sign(ax.high), f, axis, f.dim()).mul(y));
  }
  const Series own_e = we.defined() ? fe.sub(fe.mul(we)) : fe.mul(0.75), own_o = wo.defined() ? fo.sub(fo.mul(wo)) : fo.mul(0.75);
  return own_e.add(own_o).add(from_next).add(from_prev).mul(0.5);
}

struct Level final {
  std::vector<Axis> axes;
  std::vector<Series> bm, bp; // per axis; undefined = unit coefficient
  std::vector<Series> we, wo; // interpolation weights to the next-coarser level, per axis
  Series inv_diag = pluss::series::undefined();
  Series mass = pluss::series::undefined();
  Series diag = pluss::series::undefined();    // precomputed: A u = diag u - sum_a (cp_a roll(u,-1) + cm_a roll(u,+1)), wall terms folded in
  std::vector<Series> cp, cm;
  // Host copies of the stencil for the fused native smoother (filled by finalize for CPU levels that record no derivative).
  struct Host final {
    bool ok{false};
    std::int64_t n[3]{1, 1, 1}; // extents of the (padded) spatial axes, last = fastest
    std::int64_t total{0};
    std::size_t d{0};
    std::vector<double> diag, inv_diag, cp[3], cm[3];
    // Transfer weights to the next-coarser level in the shape each axis pass sees (axes before it fine, the rest coarse), full batch layout.
    bool has_w[3]{false, false, false};
    std::vector<double> we[3], wo[3];
  } host;
};

inline Series coef(const Series &b, const Series &like) { return b.defined() ? b : pluss::series::ones_like(like); }

// Precompute the fused stencil of a level: `like` has the level's field layout.
inline void finalize(Level &level, const Series &like) {
  const std::size_t d = level.axes.size();
  level.cp.assign(d, pluss::series::undefined());
  level.cm.assign(d, pluss::series::undefined());
  Series diag = pluss::series::undefined();
  for (std::size_t a = 0; a < d; ++a) {
    const std::int64_t axis = spatial_axis(like, d, a);
    const Axis &ax = level.axes[a];
    const double w = 1.0 / (ax.spacing * ax.spacing);
    const std::int64_t n = ax.cells;
    const bool hi_wall = ax.high != Edge::Periodic, lo_wall = ax.low != Edge::Periodic;
    // neighbour weights (zero where the neighbour would be across a wall: those enter through the diagonal)
    Series mask_hi = hi_wall ? axis_mask(n, n - 1, -1.0, like, axis, like.dim()).add(1.0) : Series(), mask_lo = lo_wall ? axis_mask(n, 0, -1.0, like, axis, like.dim()).add(1.0) : Series();
    Series cp = level.bp[a].defined() ? level.bp[a].mul(w) : (hi_wall ? mask_hi.mul(w) : Series());
    Series cm = level.bm[a].defined() ? level.bm[a].mul(w) : (lo_wall ? mask_lo.mul(w) : Series());
    if (level.bp[a].defined() && hi_wall)
      cp = cp.mul(mask_hi);
    if (level.bm[a].defined() && lo_wall)
      cm = cm.mul(mask_lo);
    // diagonal: w [(bp + bm) - bp s_hi e_last - bm s_lo e_first]
    Series bp = coef(level.bp[a], like), bm = coef(level.bm[a], like);
    Series term = bp.add(bm);
    if (hi_wall)
      term = term.sub(bp.mul(axis_mask(n, n - 1, sign(ax.high), like, axis, like.dim())));
    if (lo_wall)
      term = term.sub(bm.mul(axis_mask(n, 0, sign(ax.low), like, axis, like.dim())));
    term = term.mul(w);
    diag = a == 0 ? term : diag.add(term);
    // materialise full-layout weights: a same-shape multiply is several times cheaper than a broadcast one
    const auto full = [&](const Series &c, double constant) { return (c.defined() ? c : pluss::series::ones_like(like).mul(constant)).expand(like.sizes()).contiguous(); };
    level.cp[a] = full(cp, w);
    level.cm[a] = full(cm, w);
  }
  if (level.mass.defined())
    diag = diag.add(level.mass);
  level.diag = diag.expand(like.sizes()).contiguous();
  level.inv_diag = level.diag.reciprocal();
  if (like.is_cpu() && !level.diag.tracks_slope() && d <= 3) {
    const auto pull = [](const Series &x) {
      const Series cpu = x.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
      return cpu.reshape({cpu.numel()}).host<double>();
    };
    Level::Host &h = level.host;
    h.d = d;
    for (std::size_t a = 0; a < d; ++a)
      h.n[3 - d + a] = level.axes[a].cells;
    h.diag = pull(level.diag);
    h.inv_diag = pull(level.inv_diag);
    for (std::size_t a = 0; a < d; ++a) {
      h.cp[3 - d + a] = pull(level.cp[a]);
      h.cm[3 - d + a] = pull(level.cm[a]);
    }
    h.total = static_cast<std::int64_t>(h.diag.size());
    h.ok = true;
  }
}

// Runs fn(first_row, last_row) over [0, rows) on a few threads when the work is large.
template <class Fn> inline void parallel_rows(std::int64_t rows, std::int64_t work, Fn &&fn) {
  const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
  const unsigned threads = work < 100000 ? 1u : std::min<unsigned>(hw, 8u);
  if (threads == 1 || rows < 2) {
    fn(std::int64_t{0}, rows);
    return;
  }
  std::vector<std::thread> pool;
  const std::int64_t chunk = (rows + threads - 1) / threads;
  for (unsigned t = 0; t < threads; ++t) {
    const std::int64_t lo = static_cast<std::int64_t>(t) * chunk, hi = std::min(rows, lo + chunk);
    if (lo >= hi) break;
    pool.emplace_back([&fn, lo, hi] { fn(lo, hi); });
  }
  for (std::thread &th : pool) th.join();
}

// out = A u (when r == nullptr), or one fused Chebyshev step: with au = A dir,
//   r -= au; dir_new = dir * keep + r * inv_diag * gain; x += dir_new
// in a single pass over the field (the stencil, the residual update, the new direction and the iterate).
inline void stencil_pass(const Level::Host &h, const double *u, double *out, double *r, double *x, double keep, double gain) {
  const std::int64_t n0 = h.n[0], n1 = h.n[1], n2 = h.n[2], plane = n1 * n2, cells = n0 * plane;
  const std::int64_t rows = h.total / n2;
  parallel_rows(rows, h.total, [&](std::int64_t first, std::int64_t last) {
    for (std::int64_t row = first; row < last; ++row) {
      const std::int64_t j = row % n1, i = (row / n1) % n0, b = row / (n1 * n0);
      const std::int64_t base = row * n2;
      const std::int64_t up0 = (i + 1 == n0) ? base - (n0 - 1) * plane : base + plane, dn0 = (i == 0) ? base + (n0 - 1) * plane : base - plane;
      const std::int64_t up1 = (j + 1 == n1) ? base - (n1 - 1) * n2 : base + n2, dn1 = (j == 0) ? base + (n1 - 1) * n2 : base - n2;
      (void)b; (void)cells;
      for (std::int64_t k = 0; k < n2; ++k) {
        const std::int64_t idx = base + k;
        double au = h.diag[idx] * u[idx];
        if (h.d >= 3) au -= h.cp[0][idx] * u[up0 + k] + h.cm[0][idx] * u[dn0 + k];
        if (h.d >= 2) au -= h.cp[1][idx] * u[up1 + k] + h.cm[1][idx] * u[dn1 + k];
        {
          const double up = u[(k + 1 == n2) ? base : idx + 1], dn = u[(k == 0) ? base + n2 - 1 : idx - 1];
          au -= h.cp[2][idx] * up + h.cm[2][idx] * dn;
        }
        if (r == nullptr) {
          out[idx] = au;
        } else {
          const double rn = r[idx] - au;
          r[idx] = rn;
          const double dn = u[idx] * keep + rn * h.inv_diag[idx] * gain;
          out[idx] = dn;
          x[idx] += dn;
        }
      }
    }
  });
}

inline std::vector<double> pull_host(const Series &s) {
  const Series cpu = s.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return cpu.reshape({cpu.numel()}).host<double>();
}

// Chebyshev iteration for A x = b on host arrays (x0 = nullptr: zero start); the same recurrence as `chebyshev`. Returns x.
inline std::vector<double> cheb_host(const Level::Host &h, const std::vector<double> &b, const std::vector<double> *x0, std::int64_t degree, double lo, double hi) {
  const double theta = 0.5 * (hi + lo), delta = 0.5 * (hi - lo), sigma = theta / delta;
  double rho = 1.0 / sigma;
  const std::size_t total = b.size();
  std::vector<double> r = b, x(total, 0.0), dir(total), next(total);
  if (x0 != nullptr) {
    x = *x0;
    stencil_pass(h, x.data(), next.data(), nullptr, nullptr, 0.0, 0.0); // next = A x
    for (std::size_t i = 0; i < total; ++i) r[i] -= next[i];
  }
  for (std::size_t i = 0; i < total; ++i) {
    dir[i] = r[i] * h.inv_diag[i] / theta;
    x[i] += dir[i];
  }
  for (std::int64_t k = 1; k < degree; ++k) {
    const double rho_new = 1.0 / (2.0 * sigma - rho);
    stencil_pass(h, dir.data(), next.data(), r.data(), x.data(), rho_new * rho, 2.0 * rho_new / delta);
    dir.swap(next);
    rho = rho_new;
  }
  return x;
}

inline Series from_host(std::vector<double> x, const Series &like) {
  return pluss::make_series(x, std::vector<std::int64_t>(like.sizes().begin(), like.sizes().end()), pluss::Config{}.with_dtype(pluss::ScalarType::Float64)).to(like.scalar_type());
}

inline Series chebyshev_native(const Level &lv, const Series &b, const Series &x0, std::int64_t degree, double lo, double hi) {
  const std::vector<double> rhs = pull_host(b);
  if (!x0.defined())
    return from_host(cheb_host(lv.host, rhs, nullptr, degree, lo, hi), b);
  const std::vector<double> start = pull_host(x0);
  return from_host(cheb_host(lv.host, rhs, &start, degree, lo, hi), b);
}

// ---- fused host transfers: one pass per axis over (outer, n, inner) views of [batch..., spatial...] arrays, the formulas of prolong_axis / restrict_axis ----

// coarse c (outer, n, inner) -> fine out (outer, 2n, inner); we/wo (the shape of c) or nullptr for the constant 1/4 weights
inline void host_prolong_axis(const double *c, double *out, std::int64_t outer, std::int64_t n, std::int64_t inner, const Axis &ax, const double *we, const double *wo) {
  const bool per_lo = ax.low == Edge::Periodic, per_hi = ax.high == Edge::Periodic;
  const double s_lo = sign(ax.low), s_hi = sign(ax.high);
  parallel_rows(outer * n, outer * n * inner, [&](std::int64_t first, std::int64_t last) {
    for (std::int64_t row = first; row < last; ++row) {
      const std::int64_t o = row / n, i = row % n;
      for (std::int64_t j = 0; j < inner; ++j) {
        const std::int64_t idx = row * inner + j;
        const double cv = c[idx];
        const double below = i > 0 ? c[idx - inner] : (per_lo ? c[idx + (n - 1) * inner] : s_lo * cv);
        const double above = i + 1 < n ? c[idx + inner] : (per_hi ? c[idx - (n - 1) * inner] : s_hi * cv);
        const double even = we ? cv - (cv - below) * we[idx] : cv * 0.75 + below * 0.25;
        const double odd = wo ? cv - (cv - above) * wo[idx] : cv * 0.75 + above * 0.25;
        double *dst = out + (o * 2 * n + 2 * i) * inner + j;
        dst[0] = even;
        dst[inner] = odd;
      }
    }
  });
}

// fine f (outer, 2n, inner) -> coarse out (outer, n, inner): the adjoint of the prolongation, times 1/2
inline void host_restrict_axis(const double *f, double *out, std::int64_t outer, std::int64_t n, std::int64_t inner, const Axis &ax, const double *we, const double *wo) {
  const bool per = ax.high == Edge::Periodic;
  const double s_lo = sign(ax.low), s_hi = sign(ax.high);
  parallel_rows(outer * n, outer * n * inner, [&](std::int64_t first, std::int64_t last) {
    for (std::int64_t row = first; row < last; ++row) {
      const std::int64_t o = row / n, I = row % n;
      const auto fe_at = [&](std::int64_t row_, std::int64_t j) { return f[(((row_ / n) * 2 * n) + 2 * (row_ % n)) * inner + j]; };
      const auto fo_at = [&](std::int64_t row_, std::int64_t j) { return f[(((row_ / n) * 2 * n) + 2 * (row_ % n)) * inner + inner + j]; };
      const auto X = [&](std::int64_t row_, std::int64_t j) { return fe_at(row_, j) * (we ? we[row_ * inner + j] : 0.25); };
      const auto Y = [&](std::int64_t row_, std::int64_t j) { return fo_at(row_, j) * (wo ? wo[row_ * inner + j] : 0.25); };
      const std::int64_t next_row = o * n + (I + 1 < n ? I + 1 : 0), prev_row = o * n + (I > 0 ? I - 1 : n - 1);
      for (std::int64_t j = 0; j < inner; ++j) {
        const std::int64_t widx = row * inner + j;
        const double fe = fe_at(row, j), fo = fo_at(row, j);
        const double own_e = we ? fe - fe * we[widx] : fe * 0.75, own_o = wo ? fo - fo * wo[widx] : fo * 0.75;
        double from_next, from_prev;
        if (per) {
          from_next = X(next_row, j);
          from_prev = Y(prev_row, j);
        } else {
          from_next = (I + 1 < n ? X(next_row, j) : 0.0) + (I + 1 == n ? s_hi * Y(row, j) : 0.0);
          from_prev = (I > 0 ? Y(prev_row, j) : 0.0) + (I == 0 ? s_lo * X(row, j) : 0.0);
        }
        out[widx] = (own_e + own_o + from_next + from_prev) * 0.5;
      }
    }
  });
}

[[nodiscard]] inline Series apply(const Level &level, const Series &u) {
  const std::size_t d = level.axes.size();
  Series out = level.diag.mul(u);
  for (std::size_t a = 0; a < d; ++a) {
    const std::int64_t axis = spatial_axis(u, d, a);
    const Series up = u.roll(-1, axis), dn = u.roll(1, axis);
    out = out.sub(up.mul(level.cp[a]).add(dn.mul(level.cm[a])));
  }
  return out;
}

} // namespace detail

// Chebyshev polynomial iteration for A x = b with the diagonal scaling `inv_diag` (D^-1), damping the part of the spectrum of D^-1 A in [lo, hi];
// `x` is the starting guess (undefined = zero). Exposed because the same smoother serves any operator with a known diagonal and spectral bound.
template <class Apply>
[[nodiscard]] Series chebyshev(Apply &&apply, const Series &inv_diag, const Series &b, Series x, std::int64_t degree, double lo, double hi) {
  const double theta = 0.5 * (hi + lo), delta = 0.5 * (hi - lo), sigma = theta / delta;
  double rho = 1.0 / sigma;
  Series r = x.defined() ? b.sub(apply(x)) : b;
  Series dir = r.mul(inv_diag).mul(1.0 / theta);
  x = x.defined() ? x.add(dir) : dir;
  for (std::int64_t k = 1; k < degree; ++k) {
    r = r.sub(apply(dir));
    const double rho_new = 1.0 / (2.0 * sigma - rho);
    dir = dir.mul(rho_new * rho).add(r.mul(inv_diag).mul(2.0 * rho_new / delta));
    x = x.add(dir);
    rho = rho_new;
  }
  return x;
}

class Hierarchy final {
public:
  Hierarchy(Problem problem, const Series &like, Options options = {}) : options_(options) {
    using namespace detail;
    const std::size_t d = problem.axes.size();
    if (d == 0 || d > 3)
      throw std::invalid_argument("math::multigrid: expected one to three spatial axes");
    for (const Axis &ax : problem.axes) {
      if (ax.cells < 1 || !(ax.spacing > 0.0) || !std::isfinite(ax.spacing))
        throw std::invalid_argument("math::multigrid: cells must be positive and spacing finite and positive");
      if ((ax.low == Edge::Periodic) != (ax.high == Edge::Periodic))
        throw std::invalid_argument("math::multigrid: a periodic axis must be periodic at both ends");
    }
    if (!like.defined() || !like.dtype().is_floating())
      throw std::invalid_argument("math::multigrid: fields must be real floating Series");
    if (static_cast<std::size_t>(like.dim()) < d)
      throw std::invalid_argument("math::multigrid: field rank is smaller than the number of spatial axes");
    for (std::size_t a = 0; a < d; ++a)
      if (like.size(spatial_axis(like, d, a)) != problem.axes[a].cells)
        throw std::invalid_argument("math::multigrid: field extents do not match the problem axes");
    Level fine;
    fine.axes = problem.axes;
    fine.bm.assign(d, pluss::series::undefined());
    fine.bp.assign(d, pluss::series::undefined());
    singular_ = !problem.mass.defined();
    fine.mass = problem.mass;
    for (std::size_t a = 0; a < d; ++a) {
      const Series lo = a < problem.face_low.size() ? problem.face_low[a] : pluss::series::undefined();
      const Series hi = a < problem.face_high.size() ? problem.face_high[a] : pluss::series::undefined();
      const std::int64_t axis = spatial_axis(like, d, a);
      if (lo.defined() || hi.defined()) {
        for (const Series *c : {&lo, &hi})
          if (c->defined() && !c->gt(0.0).logical_and(c->isfinite()).all().item<bool>())
            throw std::invalid_argument("math::multigrid: face coefficients must be finite and positive");
        // derive the missing side from the other: b-_i = b+_{i-1}
        fine.bp[a] = hi.defined() ? hi : upper_from_lower(lo, axis, problem.axes[a]);
        fine.bm[a] = lo.defined() ? lo : lower_from_upper(hi, axis, problem.axes[a]);
      }
      if (problem.axes[a].low == Edge::Dirichlet || problem.axes[a].high == Edge::Dirichlet)
        singular_ = false;
    }
    levels_.push_back(std::move(fine));
    for (std::int64_t l = 1; l < options_.max_levels; ++l) {
      const Level &f = levels_.back();
      bool even = true;
      std::int64_t total = 1;
      for (const Axis &ax : f.axes) {
        even = even && ax.cells % 2 == 0;
        total *= ax.cells;
      }
      if (!even || total <= options_.coarsest_cells)
        break;
      levels_.push_back(coarsen(f));
      link(levels_[levels_.size() - 2], levels_.back());
    }
    for (Level &lv : levels_) {
      Series shaped = like;
      // inverse diagonal in the coarse field layout (batch axes of `like`, level extents)
      std::vector<std::int64_t> shape(like.sizes().begin(), like.sizes().end());
      for (std::size_t a = 0; a < d; ++a)
        shape[static_cast<std::size_t>(spatial_axis(like, d, a))] = lv.axes[a].cells;
      shaped = pluss::series::ones(shape, like.scalar_type()).to(like.platform());
      finalize(lv, shaped);
    }
    // dense pseudo-inverse of the coarsest operator, one matrix per lane
    {
      const Level &lv = levels_.back();
      std::int64_t total = 1;
      for (const Axis &ax : lv.axes)
        total *= ax.cells;
      if (total <= options_.dense_limit) {
        const std::int64_t batch = like.dim() - static_cast<std::int64_t>(d);
        std::vector<std::int64_t> lane_shape(like.sizes().begin(), like.sizes().begin() + batch), basis_shape{total};
        for (std::int64_t b = 0; b < batch; ++b)
          basis_shape.push_back(1);
        std::vector<std::int64_t> cells;
        for (const Axis &ax : lv.axes)
          cells.push_back(ax.cells);
        basis_shape.insert(basis_shape.end(), cells.begin(), cells.end());
        const Series basis = pluss::series::eye(total, like.scalar_type()).to(like.platform()).reshape(basis_shape);
        const Series image = detail::apply(lv, basis); // [N, B..., cells]: column k of the matrix is the image of e_k
        std::vector<std::int64_t> perm, matrix_shape(image.sizes().begin() + 1, image.sizes().begin() + 1 + batch); // lanes that carry their own coefficients
        for (std::int64_t i = 1; i <= batch + static_cast<std::int64_t>(d); ++i)
          perm.push_back(i);
        perm.push_back(0);
        matrix_shape.push_back(total);
        matrix_shape.push_back(total);
        // inverted in double precision and symmetrised so a single-precision cycle stays an exactly symmetric preconditioner
        const Series dense = image.permute(perm).contiguous().reshape(matrix_shape).to(pluss::ScalarType::Float64);
        const Series inverse = pluss::subsp::linearalg::pinverse(dense, 1e-10);
        coarse_inverse_ = inverse.add(inverse.transpose(-2, -1)).mul(0.5).to(like.scalar_type());
        coarse_cells_ = cells;
      }
    }
    build_native(like);
  }

  [[nodiscard]] std::size_t levels() const noexcept { return levels_.size(); }
  [[nodiscard]] bool singular() const noexcept { return singular_; }
  [[nodiscard]] std::int64_t coarsest_cells() const {
    std::int64_t n = 1;
    for (const auto &ax : levels_.back().axes)
      n *= ax.cells;
    return n;
  }

  // A u on the finest level (positive semi-definite sign)
  [[nodiscard]] Series apply(const Series &u) const {
    if (native_ && u.is_cpu() && !u.tracks_slope() && u.sizes().size() == fine_shape_.size() && std::equal(u.sizes().begin(), u.sizes().end(), fine_shape_.begin())) {
      const std::vector<double> v = detail::pull_host(u);
      std::vector<double> out(v.size());
      detail::stencil_pass(levels_.front().host, v.data(), out.data(), nullptr, nullptr, 0.0, 0.0);
      return detail::from_host(std::move(out), u);
    }
    return detail::apply(levels_.front(), u);
  }
  // one symmetric V-cycle: z ~ A^-1 r
  [[nodiscard]] Series vcycle(const Series &r) const {
    if (native_ && r.is_cpu() && !r.tracks_slope() && r.sizes().size() == fine_shape_.size() && std::equal(r.sizes().begin(), r.sizes().end(), fine_shape_.begin()))
      return detail::from_host(cycle_host(0, detail::pull_host(r)), r);
    return cycle(0, r);
  }

private:
  static Series upper_from_lower(const Series &lo, std::int64_t axis, const Axis &ax) { return detail::upper_neighbor(lo, axis, Axis{ax.cells, ax.spacing, ax.low == Edge::Periodic ? Edge::Periodic : Edge::Neumann, ax.high == Edge::Periodic ? Edge::Periodic : Edge::Neumann}); }
  static Series lower_from_upper(const Series &hi, std::int64_t axis, const Axis &ax) { return detail::lower_neighbor(hi, axis, Axis{ax.cells, ax.spacing, ax.low == Edge::Periodic ? Edge::Periodic : Edge::Neumann, ax.high == Edge::Periodic ? Edge::Periodic : Edge::Neumann}); }

  static detail::Level coarsen(const detail::Level &f) {
    using namespace detail;
    const std::size_t d = f.axes.size();
    Level c;
    c.axes = f.axes;
    for (Axis &ax : c.axes) {
      ax.cells /= 2;
      ax.spacing *= 2.0;
    }
    c.bm.assign(d, pluss::series::undefined());
    c.bp.assign(d, pluss::series::undefined());
    c.we.assign(d, pluss::series::undefined());
    c.wo.assign(d, pluss::series::undefined());
    if (f.mass.defined()) {
      c.mass = f.mass;
      for (std::size_t a = 0; a < d; ++a)
        c.mass = pair_mean(c.mass, spatial_axis(c.mass, d, a));
    }
    return c;
  }
  // fills the coarse conductances of `c` and the transfer weights stored on the fine level `f`
  static void link(detail::Level &f, detail::Level &c) {
    using namespace detail;
    const std::size_t d = f.axes.size();
    f.we.assign(d, pluss::series::undefined());
    f.wo.assign(d, pluss::series::undefined());
    for (std::size_t a = 0; a < d; ++a) {
      if (!f.bp[a].defined() || !f.bm[a].defined())
        continue;
      const Axis &ax = f.axes[a];
      // transverse conduction is parallel (arithmetic mean over the 2^(d-1) children), along the axis the links are in series
      Series bp = f.bp[a], bm = f.bm[a];
      for (std::size_t b = 0; b < d; ++b)
        if (b != a) {
          bp = pair_mean(bp, spatial_axis(bp, d, b));
          bm = pair_mean(bm, spatial_axis(bm, d, b));
        }
      const std::int64_t axis = spatial_axis(bp, d, a), n = ax.cells / 2;
      const Series ep = split_parity(bp, axis, 0), op = split_parity(bp, axis, 1);
      const Series em = split_parity(bm, axis, 0), om = split_parity(bm, axis, 1);
      const Series third_hi = ax.high == Edge::Periodic ? ep.roll(-1, axis) : (n == 1 ? op : cat(ep.slice(axis, 1, n), op.select(axis, n - 1).unsqueeze(axis), axis));
      const Series third_lo = ax.low == Edge::Periodic ? om.roll(1, axis) : (n == 1 ? em : cat(em.select(axis, 0).unsqueeze(axis), om.slice(axis, 0, n - 1), axis));
      const Series s_hi = ep.reciprocal().mul(0.5).add(op.reciprocal()).add(third_hi.reciprocal().mul(0.5));
      const Series s_lo = om.reciprocal().mul(0.5).add(em.reciprocal()).add(third_lo.reciprocal().mul(0.5));
      c.bp[a] = s_hi.reciprocal().mul(2.0);
      c.bm[a] = s_lo.reciprocal().mul(2.0);
      f.wo[a] = ep.reciprocal().mul(0.5).div(s_hi);
      f.we[a] = om.reciprocal().mul(0.5).div(s_lo);
    }
  }

  // Chebyshev iteration for A x = b on `lv`, starting from x0 (undefined = zero)
  Series smooth(const detail::Level &lv, const Series &b, Series x, std::int64_t degree) const {
    const double hi = 2.2;
    if (options_.native_smoother && lv.host.ok && b.is_cpu() && !b.tracks_slope() && (!x.defined() || !x.tracks_slope()))
      return detail::chebyshev_native(lv, b, x, degree, hi / options_.smoothing_ratio, hi);
    return chebyshev([&](const Series &v) { return detail::apply(lv, v); }, lv.inv_diag, b, std::move(x), degree, hi / options_.smoothing_ratio, hi);
  }

public:
  // transfer between level l and l+1 (restriction is the adjoint of the prolongation divided by 2^d)
  [[nodiscard]] Series restrict_to_coarse(std::size_t l, Series f) const {
    const detail::Level &lv = levels_[l];
    const std::size_t d = lv.axes.size();
    for (std::size_t k = d; k-- > 0;) {
      const std::int64_t rank = f.dim();
      f = detail::restrict_axis(f, detail::spatial_axis(f, d, k), lv.axes[k], detail::duplicate_fine(lv.we[k], rank, d, k), detail::duplicate_fine(lv.wo[k], rank, d, k));
    }
    return f;
  }
  [[nodiscard]] Series prolong_from_coarse(std::size_t l, Series c) const {
    const detail::Level &lv = levels_[l];
    const std::size_t d = lv.axes.size();
    for (std::size_t k = 0; k < d; ++k) {
      const std::int64_t rank = c.dim();
      c = detail::prolong_axis(c, detail::spatial_axis(c, d, k), levels_[l + 1].axes[k], detail::duplicate_fine(lv.we[k], rank, d, k), detail::duplicate_fine(lv.wo[k], rank, d, k));
    }
    return c;
  }

private:
  // Host copies of the transfer weights and the coarse inverse; native_ stays false when any piece cannot be represented.
  void build_native(const Series &like) {
    using namespace detail;
    native_ = false;
    if (!options_.native_cycle || !like.is_cpu() || like.tracks_slope())
      return;
    for (const Level &lv : levels_)
      if (!lv.host.ok)
        return;
    const std::size_t d = levels_.front().axes.size();
    fine_shape_.assign(like.sizes().begin(), like.sizes().end());
    for (std::size_t l = 0; l + 1 < levels_.size(); ++l) {
      Level &lv = levels_[l];
      const Level &co = levels_[l + 1];
      for (std::size_t k = 0; k < d; ++k) {
        if (!lv.we[k].defined() || !lv.wo[k].defined())
          continue;
        if (lv.we[k].tracks_slope() || lv.wo[k].tracks_slope())
          return;
        std::vector<std::int64_t> shape(like.sizes().begin(), like.sizes().end() - static_cast<std::ptrdiff_t>(d));
        for (std::size_t j = 0; j < d; ++j)
          shape.push_back(j < k ? 2 * co.axes[j].cells : co.axes[j].cells);
        const auto expand = [&](const Series &w) {
          const Series dup = duplicate_fine(w, w.dim(), d, k);
          return pull_host(dup.expand(shape));
        };
        lv.host.we[k] = expand(lv.we[k]);
        lv.host.wo[k] = expand(lv.wo[k]);
        lv.host.has_w[k] = true;
      }
    }
    if (coarse_inverse_.defined()) {
      if (coarse_inverse_.tracks_slope())
        return;
      std::int64_t cells = 1;
      for (const std::int64_t c : coarse_cells_)
        cells *= c;
      std::int64_t lanes = 1;
      for (std::size_t i = 0; i + d < fine_shape_.size(); ++i)
        lanes *= fine_shape_[i];
      coarse_inverse_host_ = pull_host(coarse_inverse_);
      const std::int64_t inv_lanes = static_cast<std::int64_t>(coarse_inverse_host_.size()) / (cells * cells);
      if (inv_lanes != 1 && inv_lanes != lanes)
        return;
      coarse_inverse_lanes_ = inv_lanes;
    }
    native_ = true;
  }

  // spatial extents of level l's field, fine on axes before `fine_upto` of the given level pair
  std::vector<double> restrict_host(std::size_t l, std::vector<double> f) const {
    const detail::Level &lv = levels_[l], &co = levels_[l + 1];
    const std::size_t d = lv.axes.size();
    std::int64_t batch = 1;
    for (std::size_t i = 0; i + d < fine_shape_.size(); ++i)
      batch *= fine_shape_[i];
    for (std::size_t k = d; k-- > 0;) {
      // input: axes <= k fine, > k coarse; output: axis k coarse
      std::int64_t outer = batch, inner = 1;
      for (std::size_t j = 0; j < k; ++j)
        outer *= 2 * co.axes[j].cells;
      for (std::size_t j = k + 1; j < d; ++j)
        inner *= co.axes[j].cells;
      const std::int64_t n = co.axes[k].cells;
      std::vector<double> out(static_cast<std::size_t>(outer * n * inner));
      detail::host_restrict_axis(f.data(), out.data(), outer, n, inner, lv.axes[k], lv.host.has_w[k] ? lv.host.we[k].data() : nullptr, lv.host.has_w[k] ? lv.host.wo[k].data() : nullptr);
      f.swap(out);
    }
    return f;
  }
  std::vector<double> prolong_host(std::size_t l, std::vector<double> c) const {
    const detail::Level &lv = levels_[l], &co = levels_[l + 1];
    const std::size_t d = lv.axes.size();
    std::int64_t batch = 1;
    for (std::size_t i = 0; i + d < fine_shape_.size(); ++i)
      batch *= fine_shape_[i];
    for (std::size_t k = 0; k < d; ++k) {
      // input: axes < k fine, >= k coarse; output: axis k fine
      std::int64_t outer = batch, inner = 1;
      for (std::size_t j = 0; j < k; ++j)
        outer *= 2 * co.axes[j].cells;
      for (std::size_t j = k + 1; j < d; ++j)
        inner *= co.axes[j].cells;
      const std::int64_t n = co.axes[k].cells;
      std::vector<double> out(static_cast<std::size_t>(outer * 2 * n * inner));
      detail::host_prolong_axis(c.data(), out.data(), outer, n, inner, co.axes[k], lv.host.has_w[k] ? lv.host.we[k].data() : nullptr, lv.host.has_w[k] ? lv.host.wo[k].data() : nullptr);
      c.swap(out);
    }
    return c;
  }

  std::vector<double> cycle_host(std::size_t l, const std::vector<double> &r) const {
    const detail::Level &lv = levels_[l];
    const double hi = 2.2, lo = hi / options_.smoothing_ratio;
    if (l + 1 == levels_.size() && coarse_inverse_.defined()) {
      std::int64_t cells = 1;
      for (const std::int64_t c : coarse_cells_)
        cells *= c;
      const std::int64_t lanes = static_cast<std::int64_t>(r.size()) / cells;
      std::vector<double> z(r.size());
      for (std::int64_t b = 0; b < lanes; ++b) {
        const double *inv = coarse_inverse_host_.data() + (coarse_inverse_lanes_ == 1 ? 0 : b) * cells * cells;
        for (std::int64_t i = 0; i < cells; ++i) {
          double acc = 0.0;
          for (std::int64_t j = 0; j < cells; ++j)
            acc += inv[i * cells + j] * r[static_cast<std::size_t>(b * cells + j)];
          z[static_cast<std::size_t>(b * cells + i)] = acc;
        }
      }
      return z;
    }
    if (l + 1 == levels_.size()) {
      std::int64_t extent = 1;
      for (const auto &ax : lv.axes)
        extent = std::max(extent, ax.cells);
      return detail::cheb_host(lv.host, r, nullptr, std::clamp<std::int64_t>(2 * extent, 8, 64), lo, hi);
    }
    std::vector<double> x = detail::cheb_host(lv.host, r, nullptr, options_.smoothing_degree, lo, hi);
    std::vector<double> res(r.size());
    detail::stencil_pass(lv.host, x.data(), res.data(), nullptr, nullptr, 0.0, 0.0);
    for (std::size_t i = 0; i < res.size(); ++i)
      res[i] = r[i] - res[i];
    const std::vector<double> e = cycle_host(l + 1, restrict_host(l, std::move(res)));
    const std::vector<double> up = prolong_host(l, e);
    for (std::size_t i = 0; i < x.size(); ++i)
      x[i] += up[i];
    return detail::cheb_host(lv.host, r, &x, options_.smoothing_degree, lo, hi);
  }

  Series cycle(std::size_t l, const Series &r) const {
    const detail::Level &lv = levels_[l];
    if (l + 1 == levels_.size() && coarse_inverse_.defined()) {
      std::vector<std::int64_t> flat(r.sizes().begin(), r.sizes().end() - static_cast<std::ptrdiff_t>(coarse_cells_.size()));
      std::vector<std::int64_t> full(r.sizes().begin(), r.sizes().end());
      std::int64_t total = 1;
      for (const std::int64_t c : coarse_cells_)
        total *= c;
      flat.push_back(total);
      flat.push_back(1);
      return coarse_inverse_.matmul(r.reshape(flat)).reshape(full);
    }
    if (l + 1 == levels_.size()) {
      std::int64_t extent = 1;
      for (const auto &ax : lv.axes)
        extent = std::max(extent, ax.cells);
      return smooth(lv, r, pluss::series::undefined(), std::clamp<std::int64_t>(2 * extent, 8, 64));
    }
    Series x = smooth(lv, r, pluss::series::undefined(), options_.smoothing_degree);
    Series res = r.sub(detail::apply(lv, x));
    const std::size_t d = lv.axes.size();
    res = restrict_to_coarse(l, res);
    Series e = cycle(l + 1, res);
    x = x.add(prolong_from_coarse(l, e));
    return smooth(lv, r, std::move(x), options_.smoothing_degree);
  }

  Options options_;
  bool singular_{true};
  std::vector<detail::Level> levels_;
  Series coarse_inverse_ = pluss::series::undefined();
  std::vector<std::int64_t> coarse_cells_;
  bool native_{false};
  std::vector<std::int64_t> fine_shape_;
  std::vector<double> coarse_inverse_host_;
  std::int64_t coarse_inverse_lanes_{1};
};

// Multigrid-preconditioned conjugate gradients on A x = rhs. For a singular problem the rhs must have zero mean over the spatial axes (checked by
// the caller; the solution is returned with zero spatial mean).
[[nodiscard]] inline krylov::CgResult solve(const Hierarchy &h, const Series &rhs, std::size_t spatial_dims, const krylov::CgOptions &options = {}) {
  std::vector<std::int64_t> dims;
  for (std::size_t a = 0; a < spatial_dims; ++a)
    dims.push_back(rhs.dim() - static_cast<std::int64_t>(spatial_dims) + static_cast<std::int64_t>(a));
  auto result = krylov::pcg([&](const Series &u) { return h.apply(u); }, [&](const Series &r) { return h.vcycle(r); }, rhs, dims, options);
  if (h.singular())
    result.solution = result.solution.sub(result.solution.mean(dims, true));
  return result;
}

} // namespace pluss::subsp::math::multigrid
