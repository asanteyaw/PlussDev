#pragma once

// Batched linear programming by a homogeneous self-dual interior-point method with implicit bounds.
//
//   minimise  c^T x   subject to  Aeq x = beq,  Aub x <= bub,  lower <= x <= upper
//
// The constraint matrices are shared by every lane; c, beq, bub and the bound VALUES are per lane (leading
// axes of the Series). One call therefore solves a parameter sweep (knock-outs, medium variations, every open
// node of a branch-and-bound level, the 2r objectives of flux variability analysis) in the same batched ops:
// the normal equations A D A^T are one [B, m, m] batched matmul + inverse per iteration, no per-lane host loop.
//
// Standard form: finite lower bounds are shifted out (x = l + z), variables bounded only above are flipped
// (x = u - z), free variables are split (x = z+ - z-), inequalities get slacks. Finite upper bounds z <= ubar are
// handled IMPLICITLY (a second complementarity pair w = ubar tau - z, v) so the normal equations keep the size of
// the equality block: D_j = 1 / (s_j/z_j + v_j/w_j). The method is the homogeneous self-dual embedding with Mehrotra
// predictor-corrector steps (Ye, Todd & Mizuno 1994; Xu, Hung & Ye 1996; Andersen & Andersen 2000), so it returns
// CERTIFICATES rather than only a point:
//   Optimal          primal and dual feasibility and the duality gap are below tolerance
//   PrimalInfeasible Farkas multipliers (y_eq free, y_ub <= 0):  g = Aeq^T y_eq + Aub^T y_ub  satisfies
//                    sup_{l<=x<=u} g^T x < beq^T y_eq + bub^T y_ub    (no feasible x can exist)
//   Unbounded        recession ray d:  Aub d <= 0, Aeq d = 0, d respects the bounds, c^T d < 0 (confirmed feasible)
// Accuracy: every iterate is refined against the exact Newton system (full-system iterative refinement with a primal
// proximal preconditioner), which keeps degenerate vertices from stalling. Rows of Aeq may be linearly dependent.
// Precision is that of an interior-point method (relative 1e-9 on residuals and gap), not a vertex-exact simplex answer.
// Bound PATTERN (which entries are finite) must be the same in all lanes.

#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace pluss::subsp::math::linprog {

// NearOptimal: the iteration stalled at the limit of attainable accuracy with residuals and gap below
// `accept_tolerance` (but above `tolerance`); the best iterate is returned. `success(status)` accepts both.
enum class Method : int { InteriorPoint = 0, Simplex = 1 };

enum class Status : int { Optimal = 0, PrimalInfeasible = 1, Unbounded = 2, IterationLimit = 3, NearOptimal = 4 };
inline bool success(Status st) { return st == Status::Optimal || st == Status::NearOptimal; }

struct Options final {
  Method method = Method::InteriorPoint; // Simplex: exact vertex solutions, one lane at a time (host pivoting, Series linear algebra)
  int max_iterations = 150;
  double tolerance = 1e-9;             // relative primal / dual / gap
  double ray_tolerance = 1e-8;         // certificate residual
  double accept_tolerance = 1e-6;      // NearOptimal if a stalled lane's best merit is below this
  int stall_patience = 6;              // iterations without halving the merit before a lane is declared stalled
  double regularization = 1e-12;       // relative diagonal (dual) shift of the normal equations
  double primal_regularization = 1e-9; // proximal term: D -> D/(1 + rho D) caps the dynamic range of A D A^T
  int refinement_sweeps = 4;           // full-system iterative refinement sweeps
  bool confirm_unbounded = true;       // a recession ray alone is dual infeasibility; confirm primal feasibility
  std::int64_t max_lanes = 0;          // >0: solve at most this many lanes per batched call (bounds the [B, m, m] workspace)
  double sparse_density = 0.05;        // assemble A D A^T from the non-zeros when the matrix is sparser than this (and m >= 128)
};

struct Problem final {
  Series c = pluss::series::undefined();      // [..., n]
  std::optional<Series> a_eq, b_eq;           // [me, n], [..., me]
  std::optional<Series> a_ub, b_ub;           // [mu, n], [..., mu]
  std::optional<Series> lower, upper;         // [n] or [..., n]; -inf / +inf allowed
  // Simplex warm start (shared by all lanes): a basis (standard-form column indices, one per row, from a previous `Result::basis`)
  // and the at-upper flags of the nonbasic columns. Skips phase I; bound changes are repaired by dual simplex pivots, cost changes
  // by primal pivots. An unusable basis silently falls back to a cold start.
  std::optional<std::vector<std::int64_t>> warm_basis;
  std::optional<std::vector<char>> warm_at_upper;
};

struct Result final {
  Series x = pluss::series::undefined();              // [..., n]
  Series objective = pluss::series::undefined();      // [...]
  Series dual_objective = pluss::series::undefined(); // [...] equals `objective` at an optimum (certificate)
  Series status = pluss::series::undefined();         // [...] Float64 holding Status codes
  Series dual_eq = pluss::series::undefined();        // [..., me] d objective / d beq
  Series dual_ub = pluss::series::undefined();        // [..., mu] (<= 0 for a minimisation)
  Series ray = pluss::series::undefined();            // [..., n] recession direction (meaningful when Unbounded)
  Series farkas = pluss::series::undefined();         // [..., me + mu] (y_eq, y_ub) (meaningful when PrimalInfeasible)
  int iterations = 0;
  std::vector<std::vector<std::int64_t>> basis;  // Simplex only: optimal basis of every lane (feed back as Problem::warm_basis)
  std::vector<std::vector<char>> at_upper;       // Simplex only: nonbasic at-upper flags of every lane
  std::vector<char> warm_started;                // Simplex only: 1 if the lane really started from the supplied basis (else it fell back to a cold start)
};

namespace detail {
inline Series cat_cols(const Series &a, const Series &b) { return a.unite(std::vector<Series>{b}, 1); }
inline Series cat_rows(const Series &a, const Series &b) { return a.unite(std::vector<Series>{b}, 0); }
inline Series lane_cat(const Series &a, const Series &b) { return a.unite(std::vector<Series>{b}, -1); }
inline Series dot(const Series &a, const Series &b) { return a.mul(b).sum(-1, true); } // [B,1]
inline Series amax_abs(const Series &a) { return a.abs().amax(-1, true); }
} // namespace detail

// Standard form shared by the interior-point and simplex backends:  min c_std^T z  s.t.  A z = b (rows scaled),  0 <= z <= ubar
// on the bounded set `um`, with  x = shift + T z  (finite lower bound: shift l; only upper finite: x = u - z; free: split).
struct Standard final {
  Series a = pluss::series::undefined(), b = pluss::series::undefined(), c_std = pluss::series::undefined();
  Series ubar = pluss::series::undefined(), um = pluss::series::undefined(), tmat = pluss::series::undefined();
  Series shift = pluss::series::undefined(), offset = pluss::series::undefined();
  Series row_scale = pluss::series::undefined(), col_scale = pluss::series::undefined(), c = pluss::series::undefined();
  std::int64_t n{0}, nz{0}, nn{0}, m{0}, me{0}, mu{0}, nb{0}, batch{1};
  ScalarType dt{ScalarType::Float64};
  Config cfg{};
  std::vector<std::int64_t> lead;
};

inline Standard standardize(const Problem &p) {
  using namespace detail;
  if (!p.c.defined() || p.c.dim() < 1)
    throw std::invalid_argument("linprog: c must be a [..., n] Series");
  const std::int64_t n = p.c.size(-1);
  const auto dt = p.c.scalar_type();
  const Config cfg = Config{}.with_dtype(dt).with_platform(p.c.platform());
  std::vector<std::int64_t> lead;
  for (std::int64_t i = 0; i + 1 < p.c.dim(); ++i)
    lead.push_back(p.c.size(i));
  std::int64_t batch = 1;
  for (auto s : lead)
    batch *= s;
  const Series c = p.c.reshape({batch, n});
  auto lanes = [&](const Series &v, std::int64_t k) { // broadcast [k] or [..., k] to [B, k]
    if (v.dim() == 1)
      return v.unsqueeze(0).expand({batch, k}).contiguous();
    return v.reshape({batch, k});
  };

  const double inf = std::numeric_limits<double>::infinity();
  const Series zero_n = c.mul(0.0);
  const Series lower = p.lower ? lanes(p.lower->to(dt), n) : zero_n.add(-inf);
  const Series upper = p.upper ? lanes(p.upper->to(dt), n) : zero_n.add(inf);
  auto pattern = [&](const Series &v) { // lane-uniform finite pattern as a host vector
    const Series f = v.isfinite().to(dt);
    const auto mx = f.amax(0, false).to_vec<double>(), mn = f.amin(0, false).to_vec<double>();
    std::vector<bool> out;
    for (std::int64_t j = 0; j < n; ++j) {
      if (mx[static_cast<std::size_t>(j)] != mn[static_cast<std::size_t>(j)])
        throw std::invalid_argument("linprog: the finite pattern of the bounds must be the same in every lane");
      out.push_back(mx[static_cast<std::size_t>(j)] > 0.5);
    }
    return out;
  };
  const auto low_fin = pattern(lower), up_fin = pattern(upper);

  // ---- change of variables  x = shift + T z,  z >= 0 ---------------------------------------------------
  //   finite lower: x = l + z (T = +1, shift l);  only upper finite: x = u - z (T = -1, shift u);  free: z+ - z-
  std::vector<std::int64_t> free_vars;
  std::vector<double> tdiag(static_cast<std::size_t>(n), 1.0), uses_upper(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t j = 0; j < n; ++j) {
    const bool lf = low_fin[static_cast<std::size_t>(j)], uf = up_fin[static_cast<std::size_t>(j)];
    if (!lf && uf)
      tdiag[static_cast<std::size_t>(j)] = -1.0;
    if (!lf && !uf)
      free_vars.push_back(j);
    if (lf && uf)
      uses_upper[static_cast<std::size_t>(j)] = 1.0; // bounded: z_j <= u_j - l_j handled implicitly
  }
  const std::int64_t nfree = static_cast<std::int64_t>(free_vars.size());
  Series tmat = pluss::make_series(std::vector<double>(tdiag), {n}).to(dt).to(cfg.platform).unsqueeze(-1).mul(pluss::series::eye(n, cfg));
  if (nfree > 0) {
    std::vector<double> neg(static_cast<std::size_t>(n * nfree), 0.0);
    for (std::int64_t k = 0; k < nfree; ++k)
      neg[static_cast<std::size_t>(free_vars[static_cast<std::size_t>(k)] * nfree + k)] = -1.0;
    tmat = cat_cols(tmat, pluss::make_series(neg, {n, nfree}).to(dt).to(cfg.platform));
  }
  const std::int64_t nz = n + nfree;
  const Series lower0 = lower.isfinite().where(lower, zero_n);
  const Series upper0 = upper.isfinite().where(upper, zero_n);
  std::vector<double> flip(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t j = 0; j < n; ++j)
    flip[static_cast<std::size_t>(j)] = (!low_fin[static_cast<std::size_t>(j)] && up_fin[static_cast<std::size_t>(j)]) ? 1.0 : 0.0;
  const Series flip_s = pluss::make_series(flip, {n}).to(dt).to(cfg.platform);
  const Series shift = lower0.mul(flip_s.neg().add(1.0)).add(upper0.mul(flip_s)); // [B, n]
  const Series c_std0 = c.matmul(tmat);                                             // [B, nz]
  const Series offset = dot(c, shift);                                              // [B, 1]

  const std::int64_t me = p.a_eq ? p.a_eq->size(0) : 0, mu = p.a_ub ? p.a_ub->size(0) : 0;
  const std::int64_t m = me + mu, nn = nz + mu;
  if (m == 0)
    throw std::invalid_argument("linprog: at least one equality or inequality constraint is required");
  Series a = pluss::series::undefined(), b = pluss::series::undefined();
  auto push_rows = [&](const Series &rows, const Series &rhs) {
    if (!a.defined()) {
      a = rows;
      b = rhs;
    } else {
      a = cat_rows(a, rows);
      b = lane_cat(b, rhs);
    }
  };
  if (me > 0) {
    const Series ae = p.a_eq->to(dt).to(cfg.platform);
    push_rows(cat_cols(ae.matmul(tmat), pluss::series::zeros({me, mu}, cfg)), lanes(p.b_eq->to(dt), me).sub(shift.matmul(ae.transpose(0, 1))));
  }
  if (mu > 0) {
    const Series au = p.a_ub->to(dt).to(cfg.platform);
    push_rows(cat_cols(au.matmul(tmat), pluss::series::eye(mu, cfg)), lanes(p.b_ub->to(dt), mu).sub(shift.matmul(au.transpose(0, 1))));
  }
  Series c_std = cat_cols(c_std0, pluss::series::zeros({batch, mu}, cfg)); // [B, nn]
  // implicit upper bounds on the shifted variables: mask over [nn] and bound value ubar [B, nn] (0 elsewhere)
  std::vector<double> um_host(static_cast<std::size_t>(nn), 0.0);
  std::int64_t nb = 0;
  for (std::int64_t j = 0; j < n; ++j)
    if (uses_upper[static_cast<std::size_t>(j)] > 0.5) {
      um_host[static_cast<std::size_t>(j)] = 1.0;
      ++nb;
    }
  const Series um = pluss::make_series(um_host, {nn}).to(dt).to(cfg.platform);                 // [nn]
  const Series span = upper0.sub(lower0).mul(um.slice(0, 0, n, 1));                             // [B, n]
  Series ubar = cat_cols(cat_cols(span, pluss::series::zeros({batch, nfree}, cfg)), pluss::series::zeros({batch, mu}, cfg));

  // ---- Ruiz equilibration: A' = R A C with row/column max-norms driven to 1 (shared by all lanes). The IPM runs on the scaled
  // problem (x = C x', y = R y', s = s'/C, v = v'/C); outputs are unscaled below. Without it, matrices that mix 1e-6 and 1e3
  // (big-M indicator rows, stoichiometries) stall far from the solution.
  Series row_scale = pluss::series::ones({m}, cfg), col_scale = pluss::series::ones({nn}, cfg);
  for (int sweep = 0; sweep < 12; ++sweep) {
    const Series rmax = a.abs().amax(1, false);
    const Series rs = rmax.gt(0.0).where(rmax.clamp_min(1e-300).sqrt().reciprocal(), rmax.mul(0.0).add(1.0)); // empty rows keep scale 1
    a = a.mul(rs.unsqueeze(-1));
    row_scale = row_scale.mul(rs);
    const Series cmax = a.abs().amax(0, false);
    const Series cs = cmax.gt(0.0).where(cmax.clamp_min(1e-300).sqrt().reciprocal(), cmax.mul(0.0).add(1.0)); // empty columns keep scale 1
    a = a.mul(cs.unsqueeze(0));
    col_scale = col_scale.mul(cs);
  }
  b = b.mul(row_scale);
  c_std = c_std.mul(col_scale);
  ubar = ubar.div(col_scale);
  Standard S;
  S.a = a; S.b = b; S.c_std = c_std; S.ubar = ubar; S.um = um; S.tmat = tmat; S.shift = shift; S.offset = offset;
  S.row_scale = row_scale; S.col_scale = col_scale; S.c = c;
  S.n = n; S.nz = nz; S.nn = nn; S.m = m; S.me = me; S.mu = mu; S.nb = nb; S.batch = batch; S.dt = dt; S.cfg = cfg; S.lead = lead;
  return S;
}

inline Result solve_impl(const Problem &p, const Options &opt) {
  using namespace detail;
  const Standard S = standardize(p);
  const std::int64_t n = S.n, nz = S.nz, nn = S.nn, m = S.m, me = S.me, mu = S.mu, nb = S.nb, batch = S.batch;
  const auto dt = S.dt;
  const Config cfg = S.cfg;
  const std::vector<std::int64_t> &lead = S.lead;
  const Series &a = S.a, &b = S.b, &c_std = S.c_std, &ubar = S.ubar, &um = S.um, &tmat = S.tmat, &shift = S.shift, &offset = S.offset;
  const Series &row_scale = S.row_scale, &col_scale = S.col_scale, &c = S.c;
  const Series at = a.transpose(0, 1);
  // Sparse assembly of A D A^T for large, sparse constraint matrices: M = sum_j d_j a_j a_j^T over the non-zeros of each column
  // (pair list built once on the host, then one gather + one scatter_add per iteration, O(lanes * pairs) instead of O(lanes * m^2 * n)).
  struct PairList {
    bool active{false};
    Series col = pluss::series::undefined(), target = pluss::series::undefined(), value = pluss::series::undefined();
    std::int64_t count{0};
  } pairs;
  if (m >= 128 && nn > 0) {
    const auto a_host = a.to_vec<double>();
    std::int64_t nnz = 0;
    for (double v : a_host)
      nnz += v != 0.0 ? 1 : 0;
    if (static_cast<double>(nnz) < opt.sparse_density * static_cast<double>(m) * static_cast<double>(nn)) {
      std::vector<std::vector<std::pair<std::int64_t, double>>> by_col(static_cast<std::size_t>(nn));
      for (std::int64_t i = 0; i < m; ++i)
        for (std::int64_t j = 0; j < nn; ++j) {
          const double v = a_host[static_cast<std::size_t>(i * nn + j)];
          if (v != 0.0)
            by_col[static_cast<std::size_t>(j)].emplace_back(i, v);
        }
      std::vector<double> pc, pt, pv;
      for (std::int64_t j = 0; j < nn; ++j)
        for (const auto &[ri, vi] : by_col[static_cast<std::size_t>(j)])
          for (const auto &[rk, vk] : by_col[static_cast<std::size_t>(j)]) {
            pc.push_back(static_cast<double>(j));
            pt.push_back(static_cast<double>(ri * m + rk));
            pv.push_back(vi * vk);
          }
      pairs.count = static_cast<std::int64_t>(pc.size());
      pairs.col = pluss::make_series(pc, {pairs.count}).to(ScalarType::Int64).to(cfg.platform);
      pairs.target = pluss::make_series(pt, {pairs.count}).to(ScalarType::Int64).to(cfg.platform);
      pairs.value = pluss::make_series(pv, {pairs.count}).to(dt).to(cfg.platform);
      pairs.active = pairs.count > 0;
    }
  }
  // ---- homogeneous self-dual IPM ----------------------------------------------------------------------
  Series x = pluss::series::ones({batch, nn}, cfg), s = x, w = x, v = x;
  Series y = pluss::series::zeros({batch, m}, cfg);
  Series tau = pluss::series::ones({batch, 1}, cfg), kappa = tau;
  const Series eye_m = pluss::series::eye(m, cfg);
  Series status = pluss::series::zeros({batch}, cfg).add(static_cast<double>(Status::IterationLimit));
  Series done = pluss::series::zeros({batch}, cfg);
  const Series bnorm = amax_abs(b).add(1.0), cnorm = amax_abs(c_std).add(1.0), unorm = amax_abs(ubar).add(1.0);
  const double ncomp = static_cast<double>(nn + nb + 1);
  Series best_merit = pluss::series::zeros({batch, 1}, cfg).add(1e300), stall = pluss::series::zeros({batch, 1}, cfg);
  Series best_x = x, best_s = s, best_w = w, best_v = v, best_y = y, best_tau = tau, best_kappa = kappa; // best-merit iterate per lane
  int it = 0;
  for (; it < opt.max_iterations; ++it) {
    const Series vm = v.mul(um); // bound duals, zero off the bounded set
    const Series rp = b.mul(tau).sub(x.matmul(at));                              // b tau - A x
    const Series ru = ubar.mul(tau).sub(x).sub(w).mul(um);                        // ubar tau - x - w (bounded set)
    const Series rd = y.matmul(a).add(s).sub(vm).sub(c_std.mul(tau));             // A^T y + s - v - c tau
    const Series ctx = dot(c_std, x), bty = dot(b, y).sub(dot(ubar, vm));         // dual objective b^T y - ubar^T v
    const Series rg = kappa.add(ctx).sub(bty);
    {
      const Series xs = x.div(tau), ys = y.div(tau), vs = vm.div(tau), ws = w.div(tau);
      const Series pres = amax_abs(b.sub(xs.matmul(at))).div(bnorm).maximum(amax_abs(ubar.sub(xs).sub(ws).mul(um)).div(unorm));
      const Series dres = amax_abs(ys.matmul(a).add(s.div(tau)).sub(vs).sub(c_std)).div(cnorm);
      const Series cx = dot(c_std, xs), by = dot(b, ys).sub(dot(ubar, vs));
      const Series gap = cx.sub(by).abs().div(cx.abs().add(by.abs()).add(1.0));
      const Series merit = pres.maximum(dres).maximum(gap);
      const Series improved = merit.lt(best_merit).logical_and(tau.gt(1e-12));
      // progress = merit at least halves; stalling is only meaningful close to convergence (slow early progress is normal)
      stall = merit.lt(best_merit.mul(0.5)).logical_or(best_merit.gt(1e-4)).where(stall.mul(0.0), stall.add(1.0));
      auto keep_best = [&](const Series &cur, const Series &old) { return improved.where(cur, old); };
      best_x = keep_best(x, best_x);
      best_s = keep_best(s, best_s);
      best_w = keep_best(w, best_w);
      best_v = keep_best(v, best_v);
      best_y = keep_best(y, best_y);
      best_tau = keep_best(tau, best_tau);
      best_kappa = keep_best(kappa, best_kappa);
      best_merit = improved.where(merit, best_merit);
      const Series optimal = pres.lt(opt.tolerance).logical_and(dres.lt(opt.tolerance)).logical_and(gap.lt(opt.tolerance))
                                 .logical_and(tau.gt(kappa.mul(1e-3)));
      const Series pinf = bty.gt(0.0).logical_and(amax_abs(y.matmul(a).add(s).sub(vm)).div(bty.clamp_min(1e-300)).lt(opt.ray_tolerance));
      const Series unb = ctx.lt(0.0).logical_and(amax_abs(x.matmul(at)).div(ctx.neg().clamp_min(1e-300)).lt(opt.ray_tolerance))
                             .logical_and(amax_abs(x.mul(um)).div(ctx.neg().clamp_min(1e-300)).lt(opt.ray_tolerance));
      auto mark = [&](const Series &cond, Status st) {
        const Series hit = cond.logical_and(done.unsqueeze(-1).lt(0.5)).squeeze(-1);
        status = hit.where(status.mul(0.0).add(static_cast<double>(st)), status);
        done = hit.where(done.mul(0.0).add(1.0), done);
      };
      mark(optimal, Status::Optimal);
      mark(pinf, Status::PrimalInfeasible);
      mark(unb, Status::Unbounded);
      if (it >= 10) {
        const Series stalled = stall.ge(static_cast<double>(opt.stall_patience));
        mark(stalled.logical_and(best_merit.lt(opt.accept_tolerance)), Status::NearOptimal);
        mark(stalled, Status::IterationLimit);
      }
      if (done.min().item<double>() > 0.5)
        break;
    }
    const Series active = done.unsqueeze(-1).lt(0.5);

    // -- generalised scaling and normal equations
    const Series dtrue = x.div(s.add(vm.mul(x).div(w))); // D = 1 / (s/x + v/w)  (v/w term only on the bounded set)
    // finished lanes are frozen: give them a benign D so their (unused) normal equations stay well conditioned
    const Series dvec = active.where(dtrue.div(dtrue.mul(opt.primal_regularization).add(1.0)), dtrue.mul(0.0).add(1.0));
    const Series gt = vm.mul(ubar).div(w);               // g~_j = v_j ubar_j / w_j (bounded set)
    const Series ceff = c_std.sub(gt);
    Series mraw = pluss::series::undefined();                                    // [B, m, m]  A D A^T
    if (pairs.active) {
      const Series d_cols = dvec.index_select(-1, pairs.col).mul(pairs.value);   // [B, pairs]
      const Series target = pairs.target.unsqueeze(0).expand({batch, pairs.count}).contiguous();
      mraw = pluss::series::zeros({batch, m * m}, cfg).scatter_add(1, target, d_cols).reshape({batch, m, m});
    } else {
      mraw = a.unsqueeze(0).mul(dvec.unsqueeze(-2)).matmul(at);
    }
    {
      // a lane whose normal equations are not finite (an overflowed iterate) must not poison the batch: freeze it as failed and
      // give it a harmless matrix so the factorisation of the other lanes goes through
      const Series lane_ok = mraw.isfinite().to(dt).amin(-1, false).amin(-1, false).gt(0.5); // [B]
      if (!lane_ok.all().item<bool>()) {
        mraw = lane_ok.unsqueeze(-1).unsqueeze(-1).where(mraw, eye_m.add(mraw.mul(0.0).nan_to_num(0.0, 0.0, 0.0)));
        done = lane_ok.where(done, done.mul(0.0).add(1.0));
      }
    }
    const Series diag_scale = mraw.mul(eye_m).sum(-1, true).amax(-2, true).add(1.0);
    // Cholesky of the (SPD) regularised normal equations: backward stable, unlike an explicit inverse, whose error grows
    // with the condition number and defeats the refinement below when D spans many decades.
    Series chol = pluss::series::undefined();
    {
      double shift_reg = opt.regularization;
      for (int attempt = 0;; ++attempt) { // rounding can make an extreme lane numerically indefinite: grow the shift
        try {
          chol = pluss::subsp::linearalg::cholesky(mraw.add(eye_m.mul(diag_scale).mul(shift_reg)));
          break;
        } catch (const std::exception &) {
          if (attempt >= 6)
            throw;
          shift_reg *= 100.0;
        }
      }
    }
    auto apply_minv = [&](const Series &rhs) {
      Series xsol = pluss::subsp::linearalg::cholesky_solve(chol, rhs.unsqueeze(-1));
      for (int sweep = 0; sweep < 2; ++sweep)
        xsol = xsol.add(pluss::subsp::linearalg::cholesky_solve(chol, rhs.unsqueeze(-1).sub(mraw.matmul(xsol))));
      return xsol.squeeze(-1);
    };
    const Series pvec = apply_minv(b.add(dvec.mul(ceff).matmul(at)));
    const Series uvec = dvec.mul(pvec.matmul(a).sub(ceff));
    const Series cgt = c_std.add(gt);
    const Series denom_base = dot(cgt, uvec).sub(dot(b, pvec)).sub(dot(ubar, gt));

    struct Dir { Series dx, dy, dtau, ds, dkappa, dw, dv; };
    // Solves  A dx - b dtau = r1;  dx + dw - ubar dtau = ru (bounded);  A^T dy + ds - dv - c dtau = r2;
    //         dkappa + c^T dx - b^T dy + ubar^T dv = r3;  S dx + X ds = rxs;  V dw + W dv = rwv;  kappa dtau + tau dkappa = rtk.
    auto reduce = [&](const Series &r1, const Series &ru_, const Series &r2, const Series &r3, const Series &rxs,
                      const Series &rwv, const Series &rtk) {
      const Series a_term = rwv.sub(vm.mul(ru_)).div(w).mul(um);
      const Series inner = rxs.div(x).sub(a_term).sub(r2);
      const Series qvec = apply_minv(r1.sub(dvec.mul(inner).matmul(at)));
      const Series vx = dvec.mul(qvec.matmul(a).add(inner));
      const Series num = r3.sub(dot(cgt, vx)).add(dot(b, qvec)).sub(dot(ubar, a_term)).sub(rtk.div(tau));
      const Series dtau = num.div(denom_base.sub(kappa.div(tau)));
      const Series dy = pvec.mul(dtau).add(qvec);
      const Series dx = uvec.mul(dtau).add(vx);
      const Series ds = rxs.sub(s.mul(dx)).div(x);
      const Series dw = ru_.sub(dx).add(ubar.mul(dtau)).mul(um);
      const Series dv = rwv.sub(vm.mul(dw)).div(w).mul(um);
      const Series dkappa = rtk.sub(kappa.mul(dtau)).div(tau);
      return Dir{dx, dy, dtau, ds, dkappa, dw, dv};
    };
    // Full-system iterative refinement against the EXACT Newton equations (the reduced formulas lose accuracy when D
    // spans many decades late in the solve); the proximal regularisation only shapes the preconditioner.
    auto direction = [&](const Series &rxs, const Series &rwv, const Series &rtk, const Series &rho) {
      const Series r1 = rp.mul(rho), ru_ = ru.mul(rho), r2 = rd.mul(rho).neg(), r3 = rg.mul(rho).neg();
      Dir d = reduce(r1, ru_, r2, r3, rxs, rwv, rtk);
      for (int sweep = 0; sweep < opt.refinement_sweeps; ++sweep) {
        const Series e1 = r1.sub(d.dx.matmul(at).sub(b.mul(d.dtau)));
        const Series eu = ru_.sub(d.dx.add(d.dw).sub(ubar.mul(d.dtau)).mul(um));
        const Series e2 = r2.sub(d.dy.matmul(a).add(d.ds).sub(d.dv.mul(um)).sub(c_std.mul(d.dtau)));
        const Series e3 = r3.sub(d.dkappa.add(dot(c_std, d.dx)).sub(dot(b, d.dy)).add(dot(ubar, d.dv.mul(um))));
        const Series e4 = rxs.sub(s.mul(d.dx).add(x.mul(d.ds)));
        const Series e4b = rwv.sub(vm.mul(d.dw).add(w.mul(d.dv.mul(um))).mul(um));
        const Series e5 = rtk.sub(kappa.mul(d.dtau).add(tau.mul(d.dkappa)));
        const Dir c2 = reduce(e1, eu, e2, e3, e4, e4b, e5);
        d = Dir{d.dx.add(c2.dx), d.dy.add(c2.dy), d.dtau.add(c2.dtau), d.ds.add(c2.ds), d.dkappa.add(c2.dkappa),
                d.dw.add(c2.dw), d.dv.add(c2.dv)};
      }
      return d;
    };
    auto ratio = [&](const Series &val, const Series &dval, const Series &mask) {
      return dval.lt(0.0).logical_and(mask.gt(0.5)).where(val.neg().div(dval.clamp_max(-1e-300)), dval.mul(0.0).add(1e30)).amin(-1, true);
    };
    auto max_step = [&](const Dir &d) {
      const Series ones = x.mul(0.0).add(1.0), one1 = tau.mul(0.0).add(1.0);
      return ratio(x, d.dx, ones).minimum(ratio(s, d.ds, ones)).minimum(ratio(tau, d.dtau, one1)).minimum(ratio(kappa, d.dkappa, one1))
          .minimum(ratio(w, d.dw, um.unsqueeze(0))).minimum(ratio(v, d.dv, um.unsqueeze(0)));
    };

    const Series xs_prod = x.mul(s), wv_prod = w.mul(vm);
    const Series mu_now = xs_prod.sum(-1, true).add(wv_prod.sum(-1, true)).add(tau.mul(kappa)).div(ncomp);
    const Series one_rho = tau.mul(0.0).add(1.0);
    const Dir aff = direction(xs_prod.neg(), wv_prod.neg(), tau.mul(kappa).neg(), one_rho);
    const Series alpha_aff = max_step(aff).clamp_max(1.0);
    const Series xa = x.add(aff.dx.mul(alpha_aff)), sa = s.add(aff.ds.mul(alpha_aff));
    const Series wa = w.add(aff.dw.mul(alpha_aff)), va = vm.add(aff.dv.mul(um).mul(alpha_aff));
    const Series ta = tau.add(aff.dtau.mul(alpha_aff)), ka = kappa.add(aff.dkappa.mul(alpha_aff));
    const Series mu_aff = xa.mul(sa).sum(-1, true).add(wa.mul(va).mul(um).sum(-1, true)).add(ta.mul(ka)).div(ncomp);
    const Series sigma = mu_aff.div(mu_now).pow(3.0).clamp_max(1.0);
    const Series target = sigma.mul(mu_now);
    const Series rxs = target.sub(xs_prod).sub(aff.dx.mul(aff.ds));
    const Series rwv = target.sub(wv_prod).sub(aff.dw.mul(aff.dv.mul(um))).mul(um);
    const Series rtk = target.sub(tau.mul(kappa)).sub(aff.dtau.mul(aff.dkappa));
    const Dir full = direction(rxs, rwv, rtk, sigma.neg().add(1.0));
    const Series alpha = max_step(full).mul(0.9995).clamp_max(1.0).mul(active.to(dt));
    // frozen lanes keep their iterate exactly (their direction may be NaN once x/s underflow; NaN * 0 would poison them)
    auto advance = [&](const Series &val, const Series &dval) { return active.where(val.add(dval.mul(alpha)), val); };
    x = advance(x, full.dx);
    s = advance(s, full.ds);
    y = advance(y, full.dy);
    tau = advance(tau, full.dtau);
    kappa = advance(kappa, full.dkappa);
    w = advance(w, full.dw.mul(um)).mul(um).add(um.neg().add(1.0)); // inactive entries stay at 1
    v = advance(v, full.dv.mul(um)).mul(um).add(um.neg().add(1.0));
  }

  // ---- unscale and map back to the original variables ----------------------------------------------
  {
    const Series near = status.eq(static_cast<double>(Status::NearOptimal)).unsqueeze(-1);
    x = near.where(best_x, x);
    y = near.where(best_y, y);
    v = near.where(best_v, v);
    tau = near.where(best_tau, tau);
  }
  const Series zs_scaled = x.div(tau), ys = y.div(tau), vs = v.mul(um).div(tau);
  const Series zs = zs_scaled.mul(col_scale); // unscaled standard-form variables
  const Series ys_true = ys.mul(row_scale);   // unscaled row multipliers
  const Series xo = zs.slice(-1, 0, nz, 1).matmul(tmat.transpose(0, 1)).add(shift); // [B, n]
  const Series objective = dot(c, xo).squeeze(-1);
  const Series dual_obj = dot(b, ys).sub(dot(ubar, vs)).add(offset).squeeze(-1);
  Result r;
  std::vector<std::int64_t> shape_n = lead, sh_me = lead, sh_mu = lead, sh_far = lead;
  shape_n.push_back(n);
  sh_me.push_back(me);
  sh_mu.push_back(mu);
  sh_far.push_back(m);
  auto scalar_shape = [&]() { return lead.empty() ? std::vector<std::int64_t>{} : lead; };
  r.x = xo.reshape(shape_n);
  r.objective = objective.reshape(scalar_shape());
  r.dual_objective = dual_obj.reshape(scalar_shape());
  r.status = status.reshape(scalar_shape());
  r.iterations = it;
  if (me > 0)
    r.dual_eq = ys_true.slice(-1, 0, me, 1).reshape(sh_me);
  if (mu > 0)
    r.dual_ub = ys_true.slice(-1, me, me + mu, 1).reshape(sh_mu);
  r.farkas = y.mul(row_scale).reshape(sh_far); // original-row multipliers (the bound multipliers are implied; see header)
  r.ray = x.mul(col_scale).slice(-1, 0, nz, 1).matmul(tmat.transpose(0, 1)).reshape(shape_n);
  return r;
}

// Public entry. A recession ray (c^T x < 0, A x = 0) proves DUAL infeasibility; the problem is unbounded only if it is
// also primal feasible, which is confirmed by a zero-cost solve of the same batch (a Farkas certificate replaces the
// "unbounded" status for lanes that turn out infeasible).

// ---- bounded-variable revised simplex (single lane) -------------------------------------------------------------------------
// min c^T z  s.t.  A z = b,  0 <= z <= ub  (ub = +inf off the bounded set). Phase I minimises the sum of artificial variables
// (initial basis = signed identity); phase II fixes them at zero. Dantzig pricing with a Bland fallback after a run of
// degenerate pivots, Harris two-pass ratio test, explicit dense basis inverse refactored every `kRefactor` pivots.
// Returns exact vertices: unlike an interior-point method the answer does not depend on the shape of the optimal face.
struct SimplexLane final {
  std::vector<double> z, y, v;      // structural values, row duals, bound duals
  std::vector<double> farkas, ray;  // phase-I duals (infeasible) / recession direction in z-space (unbounded)
  Status status{Status::IterationLimit};
  int iterations{0};
  std::vector<std::int64_t> basis;
  std::vector<char> at_upper;
  bool warm{false};
};

inline SimplexLane simplex_lane(const Series &a, const std::vector<double> &b, const std::vector<double> &c,
                                const std::vector<double> &ubnd, const Options &opt,
                                const std::vector<std::int64_t> *warm_basis = nullptr, const std::vector<char> *warm_upper = nullptr) {
  const std::int64_t m = a.size(0), nn = a.size(1), N = nn + m;
  const auto dt = a.scalar_type();
  const Config cfg = Config{}.with_dtype(dt).with_platform(a.platform());
  const double inf = std::numeric_limits<double>::infinity();
  const double tol = 1e-9, ptol = 1e-9;
  constexpr int kRefactor = 60;
  SimplexLane out;
  // artificial signs: residual b - A z_N with z_N = 0 is b
  std::vector<double> sign(static_cast<std::size_t>(m), 1.0);
  for (std::int64_t i = 0; i < m; ++i)
    sign[static_cast<std::size_t>(i)] = b[static_cast<std::size_t>(i)] < 0.0 ? -1.0 : 1.0;
  const Series diag_sign = pluss::make_series(sign, {m}).to(dt).to(cfg.platform).unsqueeze(-1).mul(pluss::series::eye(m, cfg));
  const Series full = a.unite(std::vector<Series>{diag_sign}, 1); // [m, N] all columns (structural + artificial)
  std::vector<double> ub(static_cast<std::size_t>(N), inf);
  std::vector<std::int64_t> basis(static_cast<std::size_t>(m)), where(static_cast<std::size_t>(N), -1);
  std::vector<double> xval(static_cast<std::size_t>(N), 0.0), xB(static_cast<std::size_t>(m));
  std::vector<char> at_upper(static_cast<std::size_t>(N), 0);
  Series binv = diag_sign;
  std::vector<double> cost(static_cast<std::size_t>(N), 0.0);
  int phase = 1, since_refactor = 0, degenerate_run = 0;
  auto cold_start = [&]() {
    for (std::int64_t j = 0; j < nn; ++j)
      ub[static_cast<std::size_t>(j)] = ubnd[static_cast<std::size_t>(j)];
    for (std::int64_t j = nn; j < N; ++j)
      ub[static_cast<std::size_t>(j)] = inf;
    std::fill(where.begin(), where.end(), -1);
    std::fill(xval.begin(), xval.end(), 0.0);
    std::fill(at_upper.begin(), at_upper.end(), 0);
    for (std::int64_t i = 0; i < m; ++i) {
      basis[static_cast<std::size_t>(i)] = nn + i;
      where[static_cast<std::size_t>(nn + i)] = i;
      xB[static_cast<std::size_t>(i)] = std::abs(b[static_cast<std::size_t>(i)]);
    }
    binv = diag_sign;
    phase = 1;
    since_refactor = 0;
    degenerate_run = 0;
  };
  auto set_phase_costs = [&]() {
    std::fill(cost.begin(), cost.end(), 0.0);
    if (phase == 1)
      for (std::int64_t i = 0; i < m; ++i)
        cost[static_cast<std::size_t>(nn + i)] = 1.0;
    else
      for (std::int64_t j = 0; j < nn; ++j)
        cost[static_cast<std::size_t>(j)] = c[static_cast<std::size_t>(j)];
  };
  cold_start();
  set_phase_costs();
  auto refactor = [&]() {
    std::vector<double> idx(basis.begin(), basis.end());
    const Series bmat = full.index_select(1, pluss::make_series(idx, {m}).to(pluss::ScalarType::Int64).to(cfg.platform));
    binv = pluss::subsp::linearalg::inverse(bmat);
    // recompute basic values from the nonbasic ones: x_B = B^{-1} (b - N x_N)
    std::vector<double> rhs(b);
    std::vector<double> nonbasic_contrib(static_cast<std::size_t>(N), 0.0);
    bool any = false;
    for (std::int64_t j = 0; j < N; ++j)
      if (where[static_cast<std::size_t>(j)] < 0 && xval[static_cast<std::size_t>(j)] != 0.0) {
        nonbasic_contrib[static_cast<std::size_t>(j)] = xval[static_cast<std::size_t>(j)];
        any = true;
      }
    Series rhs_s = pluss::make_series(rhs, {m}).to(dt).to(cfg.platform);
    if (any)
      rhs_s = rhs_s.sub(full.matmul(pluss::make_series(nonbasic_contrib, {N}).to(dt).to(cfg.platform)));
    const auto xb = binv.matmul(rhs_s.unsqueeze(-1)).squeeze(-1).to_vec<double>();
    for (std::int64_t i = 0; i < m; ++i)
      xB[static_cast<std::size_t>(i)] = xb[static_cast<std::size_t>(i)];
    since_refactor = 0;
  };
  const std::int64_t max_it = static_cast<std::int64_t>(opt.max_iterations) * 50 + 20 * (m + nn) + 1000;
  // eta update of the explicit basis inverse for a pivot on row `leave` with entering column B^{-1} a_q = alpha
  auto update_binv = [&](std::int64_t leave, const std::vector<double> &alpha) {
    // B_new^{-1} = B^{-1} - ((alpha - e_leave) / alpha_leave) (x) row_leave(B^{-1})   -- one rank-one update
    std::vector<double> eta(static_cast<std::size_t>(m));
    const double ap = alpha[static_cast<std::size_t>(leave)];
    for (std::int64_t i = 0; i < m; ++i)
      eta[static_cast<std::size_t>(i)] = (alpha[static_cast<std::size_t>(i)] - (i == leave ? 1.0 : 0.0)) / ap;
    const Series eta_s = pluss::make_series(eta, {m}).to(dt).to(cfg.platform).unsqueeze(-1);
    binv = binv.sub(eta_s.mul(binv.select(0, leave).unsqueeze(0)));
    ++since_refactor;
  };
  // ---- warm start: install the supplied basis, then dual simplex until primal feasible (bounds changed) ----------------------
  auto try_warm = [&]() -> bool {
    if (warm_basis == nullptr || static_cast<std::int64_t>(warm_basis->size()) != m)
      return false;
    std::vector<char> used(static_cast<std::size_t>(N), 0);
    std::vector<std::int64_t> resolved(warm_basis->begin(), warm_basis->end());
    for (auto &col : resolved) {
      if (col < 0)
        col = nn + (-col - 1); // artificial column of row (-col - 1)
      if (col < 0 || col >= N || used[static_cast<std::size_t>(col)])
        return false;
      used[static_cast<std::size_t>(col)] = 1;
    }
    for (std::int64_t j = 0; j < nn; ++j)
      ub[static_cast<std::size_t>(j)] = ubnd[static_cast<std::size_t>(j)];
    for (std::int64_t j = nn; j < N; ++j)
      ub[static_cast<std::size_t>(j)] = 0.0; // artificials stay out
    std::fill(where.begin(), where.end(), -1);
    std::fill(xval.begin(), xval.end(), 0.0);
    std::fill(at_upper.begin(), at_upper.end(), 0);
    for (std::int64_t i = 0; i < m; ++i) {
      basis[static_cast<std::size_t>(i)] = resolved[static_cast<std::size_t>(i)];
      where[static_cast<std::size_t>(basis[static_cast<std::size_t>(i)])] = i;
    }
    for (std::int64_t j = 0; j < nn; ++j)
      if (where[static_cast<std::size_t>(j)] < 0 && warm_upper != nullptr && static_cast<std::int64_t>(warm_upper->size()) >= nn &&
          (*warm_upper)[static_cast<std::size_t>(j)] && std::isfinite(ub[static_cast<std::size_t>(j)])) {
        at_upper[static_cast<std::size_t>(j)] = 1;
        xval[static_cast<std::size_t>(j)] = ub[static_cast<std::size_t>(j)];
      }
    phase = 2;
    set_phase_costs();
    try {
      refactor();
    } catch (const std::exception &) {
      return false;
    }
    for (std::int64_t i = 0; i < m; ++i)
      if (!std::isfinite(xB[static_cast<std::size_t>(i)]))
        return false;
    // dual simplex loop
    constexpr double ftol = 1e-9;
    for (std::int64_t it2 = 0; it2 < max_it; ++it2) {
      if (since_refactor >= kRefactor)
        refactor();
      std::int64_t r_row = -1;
      double worst = ftol;
      int dir = 0; // +1: basic value below 0 (must increase), -1: above its upper bound (must decrease)
      for (std::int64_t i = 0; i < m; ++i) {
        const double v = xB[static_cast<std::size_t>(i)], u = ub[static_cast<std::size_t>(basis[static_cast<std::size_t>(i)])];
        if (-v > worst) {
          worst = -v;
          r_row = i;
          dir = 1;
        } else if (std::isfinite(u) && v - u > worst) {
          worst = v - u;
          r_row = i;
          dir = -1;
        }
      }
      if (r_row < 0)
        return true; // primal feasible (and, by construction, dual feasible): hand over to the primal loop for the final check
      std::vector<double> cB(static_cast<std::size_t>(m));
      for (std::int64_t i = 0; i < m; ++i)
        cB[static_cast<std::size_t>(i)] = cost[static_cast<std::size_t>(basis[static_cast<std::size_t>(i)])];
      const Series pi_w = pluss::make_series(cB, {m}).to(dt).to(cfg.platform).matmul(binv);
      const auto d = pluss::make_series(std::vector<double>(cost.begin(), cost.begin() + nn), {nn}).to(dt).to(cfg.platform).sub(pi_w.matmul(a)).to_vec<double>();
      const auto row = binv.select(0, r_row).matmul(a).to_vec<double>(); // tableau row r over structural columns
      std::int64_t q = -1;
      double best_ratio = inf, best_pivot = 0.0;
      for (std::int64_t j = 0; j < nn; ++j) {
        if (where[static_cast<std::size_t>(j)] >= 0 || ub[static_cast<std::size_t>(j)] <= 0.0)
          continue;
        const double arj = row[static_cast<std::size_t>(j)];
        if (std::abs(arj) < ptol)
          continue;
        const double delta = at_upper[static_cast<std::size_t>(j)] ? -1.0 : 1.0;
        // x_Br changes by -a_rj * delta * t; increasing needs a_rj*delta < 0, decreasing needs a_rj*delta > 0
        if (dir == 1 ? !(arj * delta < 0.0) : !(arj * delta > 0.0))
          continue;
        const double ratio = std::abs(d[static_cast<std::size_t>(j)]) / std::abs(arj);
        if (ratio < best_ratio - 1e-12 || (ratio < best_ratio + 1e-12 && std::abs(arj) > best_pivot)) {
          best_ratio = ratio;
          best_pivot = std::abs(arj);
          q = j;
        }
      }
      if (q < 0)
        return false; // no entering candidate: let the cold start (phase I) classify infeasibility and build the certificate
      const std::int64_t leave_col = basis[static_cast<std::size_t>(r_row)];
      const double target = dir == 1 ? 0.0 : ub[static_cast<std::size_t>(leave_col)];
      const auto alpha = binv.matmul(full.select(1, q).unsqueeze(-1)).squeeze(-1).to_vec<double>();
      const double step = (xB[static_cast<std::size_t>(r_row)] - target) / alpha[static_cast<std::size_t>(r_row)];
      for (std::int64_t i = 0; i < m; ++i)
        xB[static_cast<std::size_t>(i)] -= alpha[static_cast<std::size_t>(i)] * step;
      const double q_value = (at_upper[static_cast<std::size_t>(q)] ? ub[static_cast<std::size_t>(q)] : 0.0) + step;
      where[static_cast<std::size_t>(leave_col)] = -1;
      at_upper[static_cast<std::size_t>(leave_col)] = dir == -1 ? 1 : 0;
      xval[static_cast<std::size_t>(leave_col)] = target;
      basis[static_cast<std::size_t>(r_row)] = q;
      where[static_cast<std::size_t>(q)] = r_row;
      at_upper[static_cast<std::size_t>(q)] = 0;
      xval[static_cast<std::size_t>(q)] = 0.0;
      xB[static_cast<std::size_t>(r_row)] = q_value;
      update_binv(r_row, alpha);
    }
    return false;
  };
  if (warm_basis != nullptr) {
    if (!try_warm()) {
      cold_start();
      set_phase_costs();
    } else {
      out.warm = true;
      // optimal-phase bookkeeping for the primal loop below
      phase = 2;
      degenerate_run = 0;
    }
  }
  Series pi_s = pluss::series::undefined();
  for (std::int64_t it = 0; it < max_it; ++it) {
    out.iterations = static_cast<int>(it);
    if (since_refactor >= kRefactor)
      refactor();
    std::vector<double> cB(static_cast<std::size_t>(m));
    for (std::int64_t i = 0; i < m; ++i)
      cB[static_cast<std::size_t>(i)] = cost[static_cast<std::size_t>(basis[static_cast<std::size_t>(i)])];
    pi_s = pluss::make_series(cB, {m}).to(dt).to(cfg.platform).matmul(binv); // y = c_B^T B^{-1}  [m]
    const auto reduced = pluss::make_series(std::vector<double>(cost.begin(), cost.begin() + nn), {nn}).to(dt).to(cfg.platform)
                             .sub(pi_s.matmul(a)).to_vec<double>(); // d_j for structural columns
    // ---- pricing
    std::int64_t q = -1;
    double best = tol;
    const bool bland = degenerate_run > 30;
    for (std::int64_t j = 0; j < nn; ++j) {
      if (where[static_cast<std::size_t>(j)] >= 0 || ub[static_cast<std::size_t>(j)] <= 0.0)
        continue; // basic or fixed at zero
      const double dj = reduced[static_cast<std::size_t>(j)];
      const double gain = at_upper[static_cast<std::size_t>(j)] ? dj : -dj; // improvement rate if moved off its bound
      if (gain > best) {
        best = bland ? tol : gain;
        q = j;
        if (bland)
          break;
      }
    }
    if (q < 0) { // optimal for this phase
      if (phase == 1) {
        double w = 0.0;
        for (std::int64_t i = 0; i < m; ++i)
          if (basis[static_cast<std::size_t>(i)] >= nn)
            w += xB[static_cast<std::size_t>(i)];
        const auto bs_scale = [&]() { double sc = 1.0; for (double v : b) sc = std::max(sc, std::abs(v)); return sc; }();
        if (w > 1e-8 * bs_scale) {
          out.status = Status::PrimalInfeasible;
          out.farkas = pi_s.to_vec<double>();
          return out;
        }
        phase = 2;
        for (std::int64_t i = 0; i < m; ++i)
          ub[static_cast<std::size_t>(nn + i)] = 0.0; // artificials may stay basic at zero but can never grow
        set_phase_costs();
        degenerate_run = 0;
        continue;
      }
      // phase II optimal: assemble the answer
      out.status = Status::Optimal;
      out.z.assign(static_cast<std::size_t>(nn), 0.0);
      for (std::int64_t j = 0; j < nn; ++j)
        out.z[static_cast<std::size_t>(j)] = where[static_cast<std::size_t>(j)] >= 0 ? xB[static_cast<std::size_t>(where[static_cast<std::size_t>(j)])] : xval[static_cast<std::size_t>(j)];
      out.basis = basis;
      for (auto &col : out.basis) // an artificial column stays basic at zero for a dependent row: encode as -(row + 1) so the basis survives a change of dimensions
        if (col >= nn)
          col = -(col - nn + 1);
      out.at_upper.assign(at_upper.begin(), at_upper.begin() + nn);
      out.y = pi_s.to_vec<double>();
      out.v.assign(static_cast<std::size_t>(nn), 0.0);
      for (std::int64_t j = 0; j < nn; ++j)
        if (std::isfinite(ub[static_cast<std::size_t>(j)]))
          out.v[static_cast<std::size_t>(j)] = std::max(0.0, -reduced[static_cast<std::size_t>(j)]);
      return out;
    }
    // ---- direction and ratio test
    const double sigma = at_upper[static_cast<std::size_t>(q)] ? -1.0 : 1.0; // +1: increase z_q, -1: decrease
    const auto alpha = binv.matmul(full.select(1, q).unsqueeze(-1)).squeeze(-1).to_vec<double>(); // B^{-1} a_q
    // x_B changes by -sigma * t * alpha
    double tmax = inf;
    for (int pass = 0; pass < 2; ++pass) {
      std::int64_t leave = -1;
      double leave_pivot = 0.0, leave_bound_is_upper = 0.0, t_choice = inf;
      for (std::int64_t i = 0; i < m; ++i) {
        const double ai = sigma * alpha[static_cast<std::size_t>(i)];
        if (std::abs(ai) < ptol)
          continue;
        const std::int64_t col = basis[static_cast<std::size_t>(i)];
        double limit;
        double upper_hit = 0.0;
        if (ai > 0.0) { // basic value decreases toward 0
          limit = (xB[static_cast<std::size_t>(i)] + (pass == 0 ? 1e-9 : 0.0)) / ai;
        } else {         // increases toward its upper bound
          const double room = ub[static_cast<std::size_t>(col)];
          if (!std::isfinite(room))
            continue;
          limit = (room - xB[static_cast<std::size_t>(i)] + (pass == 0 ? 1e-9 : 0.0)) / (-ai);
          upper_hit = 1.0;
        }
        if (pass == 0) {
          tmax = std::min(tmax, std::max(0.0, limit));
        } else if (std::max(0.0, limit) <= tmax + 1e-12) {
          const double piv = std::abs(ai);
          if (bland ? (leave < 0 || col < basis[static_cast<std::size_t>(leave)]) : piv > leave_pivot) {
            leave = i;
            leave_pivot = piv;
            leave_bound_is_upper = upper_hit;
            t_choice = std::max(0.0, limit - (ai > 0.0 ? 0.0 : 0.0));
          }
        }
      }
      if (pass == 0)
        continue;
      const double flip = ub[static_cast<std::size_t>(q)]; // entering variable reaching its opposite bound first
      double t;
      if (leave < 0) {
        if (!std::isfinite(flip)) { // nothing blocks: unbounded ray (phase II) -- cannot happen in phase I (artificial cost >= 0)
          out.status = Status::Unbounded;
          out.ray.assign(static_cast<std::size_t>(nn), 0.0);
          out.ray[static_cast<std::size_t>(q)] = sigma;
          for (std::int64_t i = 0; i < m; ++i)
            if (basis[static_cast<std::size_t>(i)] < nn)
              out.ray[static_cast<std::size_t>(basis[static_cast<std::size_t>(i)])] = -sigma * alpha[static_cast<std::size_t>(i)];
          return out;
        }
        t = flip;
      } else {
        t = std::min(t_choice, flip);
      }
      const bool bound_flip = std::isfinite(flip) && flip <= t_choice + 1e-15 && (leave < 0 || flip <= t_choice);
      degenerate_run = t < 1e-12 ? degenerate_run + 1 : 0;
      // update basic values
      for (std::int64_t i = 0; i < m; ++i)
        xB[static_cast<std::size_t>(i)] -= sigma * t * alpha[static_cast<std::size_t>(i)];
      if (bound_flip || leave < 0) {
        at_upper[static_cast<std::size_t>(q)] = at_upper[static_cast<std::size_t>(q)] ? 0 : 1;
        xval[static_cast<std::size_t>(q)] = at_upper[static_cast<std::size_t>(q)] ? flip : 0.0;
        ++since_refactor;
        break;
      }
      // basis change: entering q replaces basis[leave]
      const std::int64_t out_col = basis[static_cast<std::size_t>(leave)];
      const double entering_value = (at_upper[static_cast<std::size_t>(q)] ? flip : 0.0) + sigma * t;
      where[static_cast<std::size_t>(out_col)] = -1;
      at_upper[static_cast<std::size_t>(out_col)] = leave_bound_is_upper > 0.5 ? 1 : 0;
      xval[static_cast<std::size_t>(out_col)] = leave_bound_is_upper > 0.5 ? ub[static_cast<std::size_t>(out_col)] : 0.0;
      basis[static_cast<std::size_t>(leave)] = q;
      where[static_cast<std::size_t>(q)] = leave;
      xB[static_cast<std::size_t>(leave)] = entering_value;
      at_upper[static_cast<std::size_t>(q)] = 0;
      xval[static_cast<std::size_t>(q)] = 0.0;
      update_binv(leave, alpha);
    }
  }
  out.status = Status::IterationLimit;
  return out;
}

inline Result solve_simplex(const Problem &p, const Options &opt) {
  using namespace detail;
  const Standard S = standardize(p);
  const std::int64_t B = S.batch, nn = S.nn, m = S.m, n = S.n;
  const double inf = std::numeric_limits<double>::infinity();
  const auto b_all = S.b.to_vec<double>(), c_all = S.c_std.to_vec<double>(), u_all = S.ubar.to_vec<double>(), um_h = S.um.to_vec<double>();
  std::vector<double> x_std(static_cast<std::size_t>(B * nn), 0.0), y_all(static_cast<std::size_t>(B * m), 0.0), v_all(static_cast<std::size_t>(B * nn), 0.0);
  std::vector<double> farkas(static_cast<std::size_t>(B * m), 0.0), ray_std(static_cast<std::size_t>(B * nn), 0.0), status(static_cast<std::size_t>(B), 3.0);
  int iters = 0;
  std::vector<std::vector<std::int64_t>> bases(static_cast<std::size_t>(B));
  std::vector<std::vector<char>> uppers(static_cast<std::size_t>(B));
  std::vector<char> warm_flags(static_cast<std::size_t>(B), 0);
  for (std::int64_t l = 0; l < B; ++l) {
    std::vector<double> bl(b_all.begin() + l * m, b_all.begin() + (l + 1) * m), cl(c_all.begin() + l * nn, c_all.begin() + (l + 1) * nn);
    std::vector<double> ul(static_cast<std::size_t>(nn), inf);
    for (std::int64_t j = 0; j < nn; ++j)
      if (um_h[static_cast<std::size_t>(j)] > 0.5)
        ul[static_cast<std::size_t>(j)] = u_all[static_cast<std::size_t>(l * nn + j)];
    const auto r = simplex_lane(S.a, bl, cl, ul, opt, p.warm_basis ? &*p.warm_basis : nullptr, p.warm_at_upper ? &*p.warm_at_upper : nullptr);
    iters = std::max(iters, r.iterations);
    warm_flags[static_cast<std::size_t>(l)] = r.warm ? 1 : 0;
    status[static_cast<std::size_t>(l)] = static_cast<double>(r.status);
    if (r.status == Status::Optimal) {
      bases[static_cast<std::size_t>(l)] = r.basis;
      uppers[static_cast<std::size_t>(l)] = r.at_upper;
      std::copy(r.z.begin(), r.z.end(), x_std.begin() + l * nn);
      std::copy(r.y.begin(), r.y.end(), y_all.begin() + l * m);
      std::copy(r.v.begin(), r.v.end(), v_all.begin() + l * nn);
    } else if (r.status == Status::PrimalInfeasible) {
      std::copy(r.farkas.begin(), r.farkas.end(), farkas.begin() + l * m);
    } else if (r.status == Status::Unbounded) {
      std::copy(r.ray.begin(), r.ray.end(), ray_std.begin() + l * nn);
    }
  }
  const Series zs = pluss::make_series(x_std, {B, nn}).to(S.dt).to(S.cfg.platform);
  const Series ys = pluss::make_series(y_all, {B, m}).to(S.dt).to(S.cfg.platform);
  const Series vs = pluss::make_series(v_all, {B, nn}).to(S.dt).to(S.cfg.platform);
  const Series z_true = zs.mul(S.col_scale);
  const Series xo = z_true.slice(-1, 0, S.nz, 1).matmul(S.tmat.transpose(0, 1)).add(S.shift);
  const Series objective = dot(S.c, xo).squeeze(-1);
  const Series dual_obj = dot(S.b, ys).sub(dot(S.ubar, vs.mul(S.um))).add(S.offset).squeeze(-1);
  Result r;
  std::vector<std::int64_t> shape_n = S.lead, sh_me = S.lead, sh_mu = S.lead, sh_far = S.lead;
  shape_n.push_back(n);
  sh_me.push_back(S.me);
  sh_mu.push_back(S.mu);
  sh_far.push_back(m);
  auto scalar_shape = [&]() { return S.lead.empty() ? std::vector<std::int64_t>{} : S.lead; };
  const Series ys_true = ys.mul(S.row_scale);
  r.x = xo.reshape(shape_n);
  r.objective = objective.reshape(scalar_shape());
  r.dual_objective = dual_obj.reshape(scalar_shape());
  r.status = pluss::make_series(status, {B}).to(S.dt).to(S.cfg.platform).reshape(scalar_shape());
  r.iterations = iters;
  r.basis = std::move(bases);
  r.at_upper = std::move(uppers);
  r.warm_started = std::move(warm_flags);
  if (S.me > 0)
    r.dual_eq = ys_true.slice(-1, 0, S.me, 1).reshape(sh_me);
  if (S.mu > 0)
    r.dual_ub = ys_true.slice(-1, S.me, S.me + S.mu, 1).reshape(sh_mu);
  r.farkas = pluss::make_series(farkas, {B, m}).to(S.dt).to(S.cfg.platform).mul(S.row_scale).reshape(sh_far);
  const Series ray_z = pluss::make_series(ray_std, {B, nn}).to(S.dt).to(S.cfg.platform).mul(S.col_scale);
  r.ray = ray_z.slice(-1, 0, S.nz, 1).matmul(S.tmat.transpose(0, 1)).reshape(shape_n);
  return r;
}

inline Result solve_unchunked(const Problem &p_in, const Options &opt) {
  Problem p = p_in;
  // A bound-only problem has no rows; add one inert inequality (0 x <= 1) so the normal equations are non-empty.
  if (p.c.defined() && !p.a_eq && !p.a_ub) {
    const std::int64_t n = p.c.size(-1);
    std::vector<std::int64_t> lead;
    for (std::int64_t i = 0; i + 1 < p.c.dim(); ++i)
      lead.push_back(p.c.size(i));
    lead.push_back(1);
    p.a_ub = pluss::series::zeros({1, n}, Config{}.with_dtype(p.c.scalar_type()).with_platform(p.c.platform()));
    p.b_ub = pluss::series::ones(lead, Config{}.with_dtype(p.c.scalar_type()).with_platform(p.c.platform()));
  }
  if (opt.method == Method::Simplex)
    return solve_simplex(p, opt); // phase I decides feasibility, so unboundedness needs no confirmation solve
  Result r = solve_impl(p, opt);
  if (!opt.confirm_unbounded)
    return r;
  const Series flagged = r.status.eq(static_cast<double>(Status::Unbounded));
  if (flagged.any().item<bool>()) {
    Problem q = p;
    q.c = p.c.mul(0.0);
    Options o2 = opt;
    o2.confirm_unbounded = false;
    const Result f = solve_impl(q, o2);
    const Series infeasible = flagged.logical_and(f.status.eq(static_cast<double>(Status::PrimalInfeasible)));
    const Series unknown = flagged.logical_and(f.status.eq(static_cast<double>(Status::IterationLimit)));
    r.status = infeasible.where(r.status.mul(0.0).add(static_cast<double>(Status::PrimalInfeasible)), r.status);
    r.status = unknown.where(r.status.mul(0.0).add(static_cast<double>(Status::IterationLimit)), r.status);
    r.farkas = infeasible.unsqueeze(-1).where(f.farkas, r.farkas);
  }
  return r;
}


// Public entry. With `Options::max_lanes` set, a large batch is solved in chunks of that many lanes (the leading axes are
// flattened, per-lane fields are sliced, shared fields are reused) and the results are concatenated. The memory of one call
// is O(lanes * rows^2), so this is what makes sweeps over thousands of lanes feasible on large models.
inline Result solve(const Problem &p, const Options &opt = {}) {
  if (opt.max_lanes <= 0 || !p.c.defined() || p.c.dim() < 1)
    return solve_unchunked(p, opt);
  const std::int64_t n = p.c.size(-1);
  std::vector<std::int64_t> lead;
  std::int64_t total = 1;
  for (std::int64_t i = 0; i + 1 < p.c.dim(); ++i) {
    lead.push_back(p.c.size(i));
    total *= p.c.size(i);
  }
  if (total <= opt.max_lanes)
    return solve_unchunked(p, opt);
  auto flat = [&](const std::optional<Series> &v, std::int64_t k) -> std::optional<Series> {
    if (!v || v->dim() == 1)
      return v; // shared (matrix or 1-D bound / rhs)
    return v->reshape({total, k});
  };
  const Series c_flat = p.c.reshape({total, n});
  const auto be = flat(p.b_eq, p.b_eq ? p.b_eq->size(-1) : 0), bu = flat(p.b_ub, p.b_ub ? p.b_ub->size(-1) : 0);
  const auto lo = flat(p.lower, n), up = flat(p.upper, n);
  std::vector<Result> parts;
  for (std::int64_t start = 0; start < total; start += opt.max_lanes) {
    const std::int64_t stop = std::min(total, start + opt.max_lanes);
    Problem q;
    q.c = c_flat.slice(0, start, stop, 1);
    q.a_eq = p.a_eq;
    q.a_ub = p.a_ub;
    auto cut = [&](const std::optional<Series> &v) -> std::optional<Series> {
      if (!v || v->dim() == 1)
        return v;
      return v->slice(0, start, stop, 1);
    };
    q.b_eq = cut(be);
    q.b_ub = cut(bu);
    q.lower = cut(lo);
    q.upper = cut(up);
    Options inner = opt;
    inner.max_lanes = 0;
    parts.push_back(solve_unchunked(q, inner));
  }
  auto cat = [&](auto member) {
    Series first = (parts.front().*member);
    if (!first.defined())
      return first;
    std::vector<Series> rest;
    for (std::size_t i = 1; i < parts.size(); ++i)
      rest.push_back(parts[i].*member);
    return rest.empty() ? first : first.unite(rest, 0);
  };
  Result r;
  auto shaped = [&](Series v) {
    if (!v.defined())
      return v;
    std::vector<std::int64_t> shape = lead;
    if (v.dim() == 2)
      shape.push_back(v.size(1));
    return v.reshape(shape);
  };
  r.x = shaped(cat(&Result::x));
  r.objective = shaped(cat(&Result::objective));
  r.dual_objective = shaped(cat(&Result::dual_objective));
  r.status = shaped(cat(&Result::status));
  r.dual_eq = shaped(cat(&Result::dual_eq));
  r.dual_ub = shaped(cat(&Result::dual_ub));
  r.ray = shaped(cat(&Result::ray));
  r.farkas = shaped(cat(&Result::farkas));
  for (const auto &part : parts)
    r.iterations = std::max(r.iterations, part.iterations);
  return r;
}

} // namespace pluss::subsp::math::linprog
