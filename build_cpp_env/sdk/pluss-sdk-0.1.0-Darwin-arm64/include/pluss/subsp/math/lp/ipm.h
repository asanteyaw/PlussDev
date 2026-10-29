#pragma once

// Sparse interior-point LP solver: homogeneous self-dual embedding with Mehrotra predictor-corrector steps, implicit variable
// bounds, sparse Cholesky of the normal equations A D A^T (chol.h, minimum-degree ordering), proximal regularisation and
// full-system iterative refinement (the same algorithm as the dense batched solver in linprog.h, on sparse data), followed by an
// optional crossover to an exact vertex (crash basis from the interior solution + simplex clean-up).
//
//   min c^T x  s.t.  row_lo <= A x <= row_hi,  col_lo <= x <= col_hi
// Standard form built internally: shifted / flipped / split variables z >= 0, equality rows kept, inequality rows get a logical
// variable with its own bounds; implicit upper bounds ubar on the doubly-bounded variables.
//   Optimal, Infeasible (Farkas ray found), Unbounded (recession ray found), IterationLimit.
// References: Ye, Todd & Mizuno 1994; Xu, Hung & Ye 1996; Andersen & Andersen 2000 (MOSEK); Mehrotra 1992.

#include "pluss/subsp/math/lp/chol.h"
#include "pluss/subsp/math/lp/simplex.h"
#include "pluss/subsp/math/lp/sparse.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

namespace pluss::subsp::math::lp {

struct IpmOptions final {
  int max_iterations = 200;
  double tolerance = 1e-9;
  double regularization = 1e-12;
  double primal_regularization = 1e-9;
  int refinement_sweeps = 3;
  double time_limit_seconds = 1e30;
  const std::atomic<bool> *stop = nullptr; // cooperative cancellation (also handed to the crossover simplex)
  std::int32_t dense_column_threshold = 0; // 0 = automatic (max(200, 10% of rows)); columns above it are handled by Woodbury
  bool crossover = true;
  int verbose = 0;
  SimplexOptions simplex{};
};

struct IpmSolution final {
  SolveStatus status{SolveStatus::IterationLimit};
  double objective{0.0};
  std::vector<double> x;           // structural values (interior or vertex after crossover)
  std::vector<double> row_activity;
  std::vector<double> row_dual;    // after crossover (vertex duals); interior estimate otherwise
  int iterations{0};
  bool crossed_over{false};
  std::int64_t crossover_iterations{0};
  std::vector<VarStatus> basis; // after crossover: optimal basis (structurals then row logicals)
  double seconds{0.0};
  std::vector<double> ray;         // Unbounded
  std::vector<double> farkas;      // Infeasible (row multipliers)
};

namespace ipmdetail {

struct StdForm final {
  std::int32_t m{0}, nn{0};
  SparseMatrix a;                     // m x nn (columns of the standard form)
  std::vector<double> b, c, ubar, um; // um: 1 for bounded columns
  // mapping to original variables: x_j = shift_j + sum over its columns sign * z
  std::vector<double> shift;                       // per structural
  std::vector<std::int32_t> col_pos, col_neg;      // z+ column / z- column (-1 if none) per structural
  std::vector<double> col_sign;                    // +1 / -1 for the z+ column
  double objective_offset{0.0};
  std::vector<std::int32_t> logical_col;           // per row: logical column or -1
  std::vector<double> logical_shift, logical_sign; // per row
};

inline StdForm build_standard_form(const Model &mod) {
  StdForm S;
  const std::int32_t n = mod.cols, m = mod.rows;
  S.m = m;
  S.shift.assign(static_cast<std::size_t>(n), 0.0);
  S.col_pos.assign(static_cast<std::size_t>(n), -1);
  S.col_neg.assign(static_cast<std::size_t>(n), -1);
  S.col_sign.assign(static_cast<std::size_t>(n), 1.0);
  S.logical_col.assign(static_cast<std::size_t>(m), -1);
  S.logical_shift.assign(static_cast<std::size_t>(m), 0.0);
  S.logical_sign.assign(static_cast<std::size_t>(m), 1.0);
  std::vector<std::int32_t> tr, tc;
  std::vector<double> tv;
  S.b.assign(static_cast<std::size_t>(m), 0.0);
  auto add_col = [&](double cost, double ub) {
    const auto id = static_cast<std::int32_t>(S.c.size());
    S.c.push_back(cost);
    S.ubar.push_back(ub);
    S.um.push_back(std::isinf(ub) ? 0.0 : 1.0);
    return id;
  };
  std::vector<double> x_shift_contrib(static_cast<std::size_t>(m), 0.0); // A * shift
  for (std::int32_t j = 0; j < n; ++j) {
    const auto sj = static_cast<std::size_t>(j);
    const double lo = mod.col_lo[sj], hi = mod.col_hi[sj];
    const bool lf = !std::isinf(lo), uf = !std::isinf(hi);
    double sign = 1.0, shift = 0.0, ub = kInf;
    if (lf) {
      shift = lo;
      ub = uf ? hi - lo : kInf;
    } else if (uf) {
      sign = -1.0;
      shift = hi;
    }
    S.shift[sj] = shift;
    S.col_sign[sj] = sign;
    S.objective_offset += mod.cost[sj] * shift;
    const auto cp = add_col(mod.cost[sj] * sign, ub);
    S.col_pos[sj] = cp;
    for (std::int64_t p = mod.a.start[sj]; p < mod.a.start[sj + 1]; ++p) {
      tr.push_back(mod.a.index[static_cast<std::size_t>(p)]);
      tc.push_back(cp);
      tv.push_back(mod.a.value[static_cast<std::size_t>(p)] * sign);
      x_shift_contrib[static_cast<std::size_t>(mod.a.index[static_cast<std::size_t>(p)])] += mod.a.value[static_cast<std::size_t>(p)] * shift;
    }
    if (!lf && !uf) { // free: x = z+ - z-
      const auto cn = add_col(-mod.cost[sj], kInf);
      S.col_neg[sj] = cn;
      for (std::int64_t p = mod.a.start[sj]; p < mod.a.start[sj + 1]; ++p) {
        tr.push_back(mod.a.index[static_cast<std::size_t>(p)]);
        tc.push_back(cn);
        tv.push_back(-mod.a.value[static_cast<std::size_t>(p)]);
      }
    }
  }
  for (std::int32_t i = 0; i < m; ++i) {
    const auto si = static_cast<std::size_t>(i);
    const double lo = mod.row_lo[si], hi = mod.row_hi[si];
    if (lo == hi && !std::isinf(lo)) {
      S.b[si] = lo - x_shift_contrib[si];
      continue;
    }
    // logical l_i in [lo, hi]:  A x - l = 0  ->  A T z - sign * z_l = shift_l - A shift
    const bool lf = !std::isinf(lo), uf = !std::isinf(hi);
    double sign = 1.0, shift = 0.0, ub = kInf;
    if (lf) {
      shift = lo;
      ub = uf ? hi - lo : kInf;
    } else if (uf) {
      sign = -1.0;
      shift = hi;
    }
    const auto lc = add_col(0.0, ub);
    S.logical_col[si] = lc;
    S.logical_shift[si] = shift;
    S.logical_sign[si] = sign;
    tr.push_back(i);
    tc.push_back(lc);
    tv.push_back(-sign);
    S.b[si] = shift - x_shift_contrib[si];
    if (!lf && !uf) { // a free row: split like a free variable
      const auto ln = add_col(0.0, kInf);
      tr.push_back(i);
      tc.push_back(ln);
      tv.push_back(1.0);
    }
  }
  S.nn = static_cast<std::int32_t>(S.c.size());
  S.a = SparseMatrix::from_triplets(m, S.nn, tr, tc, tv);
  return S;
}

} // namespace ipmdetail

class InteriorPoint final {
public:
  InteriorPoint(const Model &model, const IpmOptions &opt) : model_(model), opt_(opt) { model_.validate(); model_.canonicalize_infinities(); }

  IpmSolution solve() {
    const auto t0 = std::chrono::steady_clock::now();
    IpmSolution out;
    S_ = ipmdetail::build_standard_form(model_);
    const std::int32_t m = S_.m, nn = S_.nn;
    if (m == 0) {
      out.status = SolveStatus::IterationLimit;
      return out;
    }
    scale();
    prepare_normal_equations();
    run(out);
    out.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (out.status == SolveStatus::Optimal) {
      extract(out);
      if (opt_.crossover)
        crossover(out);
    }
    (void)nn;
    return out;
  }

private:
  using Vec = std::vector<double>;

  // ---------------------------------------------------------------- scaling (Ruiz) and structure
  void scale() {
    const auto m = static_cast<std::size_t>(S_.m), nn = static_cast<std::size_t>(S_.nn);
    row_scale_.assign(m, 1.0);
    col_scale_.assign(nn, 1.0);
    for (int sweep = 0; sweep < 10; ++sweep) {
      Vec rmax(m, 0.0);
      for (std::size_t j = 0; j < nn; ++j)
        for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
          rmax[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])] = std::max(rmax[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])], std::abs(S_.a.value[static_cast<std::size_t>(p)]));
      for (std::size_t i = 0; i < m; ++i)
        if (rmax[i] > 0)
          rmax[i] = 1.0 / std::sqrt(rmax[i]);
        else
          rmax[i] = 1.0;
      for (std::size_t j = 0; j < nn; ++j)
        for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
          S_.a.value[static_cast<std::size_t>(p)] *= rmax[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])];
      for (std::size_t i = 0; i < m; ++i)
        row_scale_[i] *= rmax[i];
      for (std::size_t j = 0; j < nn; ++j) {
        double cmax = 0.0;
        for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
          cmax = std::max(cmax, std::abs(S_.a.value[static_cast<std::size_t>(p)]));
        const double cs = cmax > 0 ? 1.0 / std::sqrt(cmax) : 1.0;
        for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
          S_.a.value[static_cast<std::size_t>(p)] *= cs;
        col_scale_[j] *= cs;
      }
    }
    for (std::size_t i = 0; i < m; ++i)
      S_.b[i] *= row_scale_[i];
    for (std::size_t j = 0; j < nn; ++j) {
      S_.c[j] *= col_scale_[j];
      if (S_.um[j] > 0.5)
        S_.ubar[j] /= col_scale_[j];
    }
  }

  // sparse pattern of M = A D A^T (plus diagonal) and the index map for fast value assembly; dense columns are set aside
  void prepare_normal_equations() {
    const auto m = static_cast<std::size_t>(S_.m), nn = static_cast<std::size_t>(S_.nn);
    const std::int32_t thr = opt_.dense_column_threshold > 0 ? opt_.dense_column_threshold : std::max<std::int32_t>(200, S_.m / 10);
    is_dense_.assign(nn, 0);
    dense_cols_.clear();
    for (std::size_t j = 0; j < nn; ++j)
      if (S_.a.start[j + 1] - S_.a.start[j] > thr) {
        is_dense_[j] = 1;
        dense_cols_.push_back(static_cast<std::int32_t>(j));
      }
    // pair list: for each sparse column, all ordered pairs (i1, i2) -> accumulate into M(i1, i2)
    std::vector<std::vector<std::int32_t>> rows_cols(m); // row i -> neighbours (pattern)
    for (std::size_t j = 0; j < nn; ++j) {
      if (is_dense_[j])
        continue;
      for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
        for (std::int64_t q = S_.a.start[j]; q < S_.a.start[j + 1]; ++q)
          rows_cols[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])].push_back(S_.a.index[static_cast<std::size_t>(q)]);
    }
    Mstart_.assign(1, 0);
    Midx_.clear();
    for (std::size_t i = 0; i < m; ++i) {
      rows_cols[i].push_back(static_cast<std::int32_t>(i)); // diagonal always present
      std::sort(rows_cols[i].begin(), rows_cols[i].end());
      rows_cols[i].erase(std::unique(rows_cols[i].begin(), rows_cols[i].end()), rows_cols[i].end());
      Midx_.insert(Midx_.end(), rows_cols[i].begin(), rows_cols[i].end());
      Mstart_.push_back(static_cast<std::int64_t>(Midx_.size()));
    }
    Mval_.assign(Midx_.size(), 0.0);
    // position of M(i1, i2) inside column i2? M symmetric: store column-wise: column i2 holds rows i1. rows_cols[i1] lists i2, symmetric -> same lists
    chol_.analyze(S_.m, Mstart_, Midx_);
    dense_ready_ = true;
    if (opt_.verbose)
      std::fprintf(stderr, "ipm: m=%d nn=%d nnz(A)=%lld dense columns %zu, nnz(M)=%zu, nnz(L)=%lld\n", S_.m, S_.nn, static_cast<long long>(S_.a.nnz()), dense_cols_.size(), Midx_.size(), static_cast<long long>(chol_.stats().factor_nnz));
  }

  // M(values) = A_sparse diag(d) A_sparse^T
  void assemble(const Vec &d) {
    std::fill(Mval_.begin(), Mval_.end(), 0.0);
    const auto m = static_cast<std::size_t>(S_.m);
    // per-column position lookup via a work array keyed by row for each output column
    std::vector<std::int64_t> where(m, -1);
    // accumulate column-wise: for column c of M (a row index i2), contributions from every A column j that touches i2
    // build A^T pattern once for speed
    if (at_.rows == 0)
      at_ = S_.a.transposed(); // rows of A as sparse lists of columns
    for (std::size_t i2 = 0; i2 < m; ++i2) {
      for (std::int64_t p = Mstart_[i2]; p < Mstart_[i2 + 1]; ++p)
        where[static_cast<std::size_t>(Midx_[static_cast<std::size_t>(p)])] = p;
      for (std::int64_t pp = at_.start[i2]; pp < at_.start[i2 + 1]; ++pp) {
        const auto j = static_cast<std::size_t>(at_.index[static_cast<std::size_t>(pp)]);
        if (is_dense_[j])
          continue;
        const double w = d[j] * at_.value[static_cast<std::size_t>(pp)];
        for (std::int64_t q = S_.a.start[j]; q < S_.a.start[j + 1]; ++q)
          Mval_[static_cast<std::size_t>(where[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(q)])])] += w * S_.a.value[static_cast<std::size_t>(q)];
      }
    }
  }

  // ---------------------------------------------------------------- linear algebra helpers
  void a_times(const Vec &z, Vec &y) const { // y = A z
    std::fill(y.begin(), y.end(), 0.0);
    for (std::size_t j = 0; j < static_cast<std::size_t>(S_.nn); ++j) {
      const double zj = z[j];
      if (zj == 0.0)
        continue;
      for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
        y[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])] += S_.a.value[static_cast<std::size_t>(p)] * zj;
    }
  }
  void at_times(const Vec &y, Vec &z) const { // z = A^T y
    for (std::size_t j = 0; j < static_cast<std::size_t>(S_.nn); ++j) {
      double s = 0.0;
      for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
        s += S_.a.value[static_cast<std::size_t>(p)] * y[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])];
      z[j] = s;
    }
  }
  static double dot(const Vec &a, const Vec &b) {
    double s = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
      s += a[i] * b[i];
    return s;
  }
  static double amax_abs(const Vec &a) {
    double s = 0.0;
    for (const double v : a)
      s = std::max(s, std::abs(v));
    return s;
  }

  // solve (A_s D A_s^T + reg I + sum_dense d_k a_k a_k^T) u = rhs using the sparse factor + Woodbury for dense columns
  void solve_normal(const Vec &d_dense_unused, Vec &rhs) {
    (void)d_dense_unused;
    chol_.solve(rhs);
    if (dense_cols_.empty() || !dense_ready_)
      return;
    // Woodbury: (M_s + U D U^T)^{-1} r = M_s^{-1} r - M_s^{-1} U (D^{-1} + U^T M_s^{-1} U)^{-1} U^T M_s^{-1} r
    const std::size_t K = dense_cols_.size();
    Vec ut(K, 0.0);
    for (std::size_t k = 0; k < K; ++k) {
      const auto j = static_cast<std::size_t>(dense_cols_[k]);
      double s = 0.0;
      for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
        s += S_.a.value[static_cast<std::size_t>(p)] * rhs[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])];
      ut[k] = s;
    }
    // small K x K dense system (precomputed in factor step): solve S w = ut
    Vec w = ut;
    dense_small_solve(w);
    for (std::size_t k = 0; k < K; ++k) {
      const auto j = static_cast<std::size_t>(dense_cols_[k]);
      for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
        rhs[static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])] -= minv_u_[k][static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])] * 0.0;
    }
    for (std::size_t k = 0; k < K; ++k)
      for (std::size_t i = 0; i < rhs.size(); ++i)
        rhs[i] -= minv_u_[k][i] * w[k];
  }

  void dense_small_solve(Vec &v) const {
    const std::size_t K = dense_cols_.size();
    // Gaussian elimination with partial pivoting on the stored K x K matrix (copy)
    std::vector<Vec> A = small_;
    Vec b = v;
    for (std::size_t c = 0; c < K; ++c) {
      std::size_t piv = c;
      for (std::size_t r = c + 1; r < K; ++r)
        if (std::abs(A[r][c]) > std::abs(A[piv][c]))
          piv = r;
      std::swap(A[c], A[piv]);
      std::swap(b[c], b[piv]);
      const double d = A[c][c] != 0.0 ? A[c][c] : 1e-300;
      for (std::size_t r = c + 1; r < K; ++r) {
        const double f = A[r][c] / d;
        for (std::size_t cc = c; cc < K; ++cc)
          A[r][cc] -= f * A[c][cc];
        b[r] -= f * b[c];
      }
    }
    for (std::size_t c = K; c-- > 0;) {
      double s = b[c];
      for (std::size_t cc = c + 1; cc < K; ++cc)
        s -= A[c][cc] * v[cc];
      v[c] = s / (A[c][c] != 0.0 ? A[c][c] : 1e-300);
    }
  }

  // factor M_s + reg I (+ Woodbury data for dense columns); returns the matrix with regularised diagonal in Mval_
  void factor(const Vec &d, double reg) {
    assemble(d);
    // diagonal regularisation proportional to the largest diagonal
    double dmax = 1.0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(S_.m); ++i)
      for (std::int64_t p = Mstart_[i]; p < Mstart_[i + 1]; ++p)
        if (Midx_[static_cast<std::size_t>(p)] == static_cast<std::int32_t>(i))
          dmax = std::max(dmax, Mval_[static_cast<std::size_t>(p)]);
    Mraw_ = Mval_;
    for (std::size_t i = 0; i < static_cast<std::size_t>(S_.m); ++i)
      for (std::int64_t p = Mstart_[i]; p < Mstart_[i + 1]; ++p)
        if (Midx_[static_cast<std::size_t>(p)] == static_cast<std::int32_t>(i))
          Mval_[static_cast<std::size_t>(p)] += reg * dmax;
    chol_.factor(Mstart_, Midx_, Mval_);
    if (!dense_cols_.empty()) {
      const std::size_t K = dense_cols_.size();
      minv_u_.assign(K, Vec(static_cast<std::size_t>(S_.m), 0.0));
      small_.assign(K, Vec(K, 0.0));
      for (std::size_t k = 0; k < K; ++k) {
        const auto j = static_cast<std::size_t>(dense_cols_[k]);
        for (std::int64_t p = S_.a.start[j]; p < S_.a.start[j + 1]; ++p)
          minv_u_[k][static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])] = S_.a.value[static_cast<std::size_t>(p)];
        chol_.solve(minv_u_[k]);
      }
      for (std::size_t k = 0; k < K; ++k) {
        const auto jk = static_cast<std::size_t>(dense_cols_[k]);
        for (std::size_t l = 0; l < K; ++l) {
          double s = 0.0;
          for (std::int64_t p = S_.a.start[jk]; p < S_.a.start[jk + 1]; ++p)
            s += S_.a.value[static_cast<std::size_t>(p)] * minv_u_[l][static_cast<std::size_t>(S_.a.index[static_cast<std::size_t>(p)])];
          small_[k][l] = s;
        }
        small_[k][k] += 1.0 / std::max(d[jk], 1e-300);
      }
    }
  }

  // matvec with the exact (unregularised, sparse part) M: y = A_s D A_s^T x
  void m_times(const Vec &d, const Vec &x, Vec &y) const {
    Vec t(static_cast<std::size_t>(S_.nn), 0.0);
    at_times(x, t);
    for (std::size_t j = 0; j < t.size(); ++j)
      t[j] *= (is_dense_[j] ? d[j] : d[j]);
    a_times(t, y);
  }

  // Newton system solves ------------------------------------------------------------------------------------------
  struct Dir {
    Vec dx, dy, ds, dw, dv;
    double dtau{0}, dkappa{0};
  };

  void run(IpmSolution &out) {
    const auto m = static_cast<std::size_t>(S_.m), nn = static_cast<std::size_t>(S_.nn);
    const auto t0 = std::chrono::steady_clock::now();
    x_.assign(nn, 1.0);
    s_ = x_;
    w_ = x_;
    v_ = x_;
    y_.assign(m, 0.0);
    tau_ = 1.0;
    kappa_ = 1.0;
    Vec um(S_.um);
    std::size_t nb = 0;
    for (const double u : um)
      nb += u > 0.5;
    const double ncomp = static_cast<double>(nn + nb + 1);
    const double bnorm = 1.0 + amax_abs(S_.b), cnorm = 1.0 + amax_abs(S_.c);
    Vec ubar_safe(nn, 0.0);
    for (std::size_t j = 0; j < nn; ++j)
      ubar_safe[j] = um[j] > 0.5 ? S_.ubar[j] : 0.0;
    const double unorm = 1.0 + amax_abs(ubar_safe);
    Vec rp(m), rd(nn), ru(nn), vm(nn), t1(m), t2(nn), dvec(nn), dtrue(nn), gt(nn), ceff(nn), pvec(m), uvec(nn);
    double best_merit = 1e300;
    int stall = 0;
    Vec best_x = x_, best_s = s_, best_w = w_, best_v = v_, best_y = y_;
    double best_tau = tau_, best_kappa = kappa_;
    for (int it = 0; it < opt_.max_iterations; ++it) {
      out.iterations = it;
      if (std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() > opt_.time_limit_seconds || (opt_.stop && opt_.stop->load(std::memory_order_relaxed))) {
        out.status = SolveStatus::TimeLimit;
        break;
      }
      for (std::size_t j = 0; j < nn; ++j)
        vm[j] = um[j] > 0.5 ? v_[j] : 0.0;
      // residuals
      a_times(x_, t1);
      for (std::size_t i = 0; i < m; ++i)
        rp[i] = S_.b[i] * tau_ - t1[i];
      at_times(y_, t2);
      for (std::size_t j = 0; j < nn; ++j) {
        rd[j] = t2[j] + s_[j] - vm[j] - S_.c[j] * tau_;
        ru[j] = um[j] > 0.5 ? (ubar_safe[j] * tau_ - x_[j] - w_[j]) : 0.0;
      }
      const double ctx = dot(S_.c, x_), bty = dot(S_.b, y_) - dot(ubar_safe, vm);
      const double rg = kappa_ + ctx - bty;
      // convergence on the scaled iterate
      {
        Vec xs(nn), ys(m);
        for (std::size_t j = 0; j < nn; ++j)
          xs[j] = x_[j] / tau_;
        for (std::size_t i = 0; i < m; ++i)
          ys[i] = y_[i] / tau_;
        Vec axs(m), aty(nn);
        a_times(xs, axs);
        at_times(ys, aty);
        double pres = 0.0, dres = 0.0, bres = 0.0;
        for (std::size_t i = 0; i < m; ++i)
          pres = std::max(pres, std::abs(S_.b[i] - axs[i]));
        for (std::size_t j = 0; j < nn; ++j) {
          dres = std::max(dres, std::abs(aty[j] + s_[j] / tau_ - vm[j] / tau_ - S_.c[j]));
          if (um[j] > 0.5)
            bres = std::max(bres, std::abs(ubar_safe[j] - xs[j] - w_[j] / tau_));
        }
        pres = std::max(pres / bnorm, bres / unorm);
        dres /= cnorm;
        const double cx = dot(S_.c, xs), by = dot(S_.b, ys) - dot(ubar_safe, vm) / tau_;
        const double gap = std::abs(cx - by) / (1.0 + std::abs(cx) + std::abs(by));
        const double merit = std::max({pres, dres, gap});
        if (opt_.verbose)
          std::fprintf(stderr, "  ipm it %d: pres %.2e dres %.2e gap %.2e tau %.3e kappa %.3e\n", it, pres, dres, gap, tau_, kappa_);
        // stalling is only meaningful close to convergence (slow early progress is normal)
        if (merit < best_merit * 0.5 || best_merit > 1e-4) {
          stall = 0;
        } else {
          ++stall;
        }
        if (merit < best_merit && tau_ > 1e-12) {
          best_merit = merit;
          best_x = x_;
          best_s = s_;
          best_w = w_;
          best_v = v_;
          best_y = y_;
          best_tau = tau_;
          best_kappa = kappa_;
        }
        if (pres < opt_.tolerance && dres < opt_.tolerance && gap < opt_.tolerance && tau_ > 1e-3 * kappa_) {
          out.status = SolveStatus::Optimal;
          break;
        }
        // certificates
        Vec asum(nn);
        for (std::size_t j = 0; j < nn; ++j)
          asum[j] = t2[j] + s_[j] - vm[j];
        if (bty > 0 && amax_abs(asum) / bty < 1e-8) {
          out.status = SolveStatus::Infeasible;
          out.farkas.assign(m, 0.0);
          for (std::size_t i = 0; i < m; ++i)
            out.farkas[i] = y_[i] * row_scale_[i] / bty;
          break;
        }
        a_times(x_, t1);
        if (ctx < 0 && amax_abs(t1) / -ctx < 1e-8) {
          bool bounded_zero = true;
          for (std::size_t j = 0; j < nn; ++j)
            if (um[j] > 0.5 && std::abs(x_[j]) / -ctx > 1e-8)
              bounded_zero = false;
          if (bounded_zero) {
            out.status = SolveStatus::Unbounded;
            ray_z_.assign(nn, 0.0);
            for (std::size_t j = 0; j < nn; ++j)
              ray_z_[j] = x_[j] * col_scale_[j] / -ctx;
            break;
          }
        }
        if (stall >= 25 && best_merit < 1e-6 && it > 10) { // accuracy-limited: return the best iterate
          x_ = best_x; s_ = best_s; w_ = best_w; v_ = best_v; y_ = best_y; tau_ = best_tau; kappa_ = best_kappa;
          out.status = SolveStatus::Optimal;
          break;
        }
        if (stall >= 25)
          break;
      }
      // ---- direction machinery
      for (std::size_t j = 0; j < nn; ++j) {
        const double sj = s_[j], xj = x_[j];
        dtrue[j] = um[j] > 0.5 ? 1.0 / (sj / xj + v_[j] / w_[j]) : xj / sj;
        dvec[j] = dtrue[j] / (1.0 + opt_.primal_regularization * dtrue[j]);
        gt[j] = um[j] > 0.5 ? v_[j] * ubar_safe[j] / w_[j] : 0.0;
        ceff[j] = S_.c[j] - gt[j];
      }
      factor(dvec, opt_.regularization);
      // M^{-1} with refinement against the unregularised sparse M plus dense columns (exact for D_reg)
      auto apply_minv = [&](const Vec &rhs) {
        Vec xsol = rhs;
        solve_normal(dvec, xsol);
        const double rnorm = 1e-30 + amax_abs(rhs);
        for (int sweep = 0; sweep < 2; ++sweep) {
          Vec r = rhs, mx(m);
          m_times_full(dvec, xsol, mx);
          for (std::size_t i = 0; i < m; ++i)
            r[i] -= mx[i];
          if (amax_abs(r) <= 1e-10 * rnorm)
            break; // accurate enough: skip the extra solve
          solve_normal(dvec, r);
          for (std::size_t i = 0; i < m; ++i)
            xsol[i] += r[i];
        }
        return xsol;
      };
      Vec dcw(nn);
      for (std::size_t j = 0; j < nn; ++j)
        dcw[j] = dvec[j] * ceff[j];
      Vec adc(m);
      a_times(dcw, adc);
      Vec gvec(m);
      for (std::size_t i = 0; i < m; ++i)
        gvec[i] = S_.b[i] + adc[i];
      pvec = apply_minv(gvec);
      at_times(pvec, t2);
      for (std::size_t j = 0; j < nn; ++j)
        uvec[j] = dvec[j] * (t2[j] - ceff[j]);
      Vec cgt(nn);
      for (std::size_t j = 0; j < nn; ++j)
        cgt[j] = S_.c[j] + gt[j];
      const double denom_base = dot(cgt, uvec) - dot(S_.b, pvec) - dot(ubar_safe, gt);
      auto reduce = [&](const Vec &r1, const Vec &ru_, const Vec &r2, double r3, const Vec &rxs, const Vec &rwv, double rtk) {
        Dir d;
        d.dx.assign(nn, 0.0); d.ds.assign(nn, 0.0); d.dw.assign(nn, 0.0); d.dv.assign(nn, 0.0); d.dy.assign(m, 0.0);
        Vec a_term(nn, 0.0), inner(nn), vx(nn);
        for (std::size_t j = 0; j < nn; ++j) {
          if (um[j] > 0.5)
            a_term[j] = (rwv[j] - vm[j] * ru_[j]) / w_[j];
          inner[j] = rxs[j] / x_[j] - a_term[j] - r2[j];
        }
        Vec t(nn);
        for (std::size_t j = 0; j < nn; ++j)
          t[j] = dvec[j] * inner[j];
        Vec at_(m);
        a_times(t, at_);
        Vec h(m);
        for (std::size_t i = 0; i < m; ++i)
          h[i] = r1[i] - at_[i];
        Vec qvec = apply_minv(h);
        at_times(qvec, t2);
        for (std::size_t j = 0; j < nn; ++j)
          vx[j] = dvec[j] * (t2[j] + inner[j]);
        const double num = r3 - dot(cgt, vx) + dot(S_.b, qvec) - dot(ubar_safe, a_term) - rtk / tau_;
        d.dtau = num / (denom_base - kappa_ / tau_);
        for (std::size_t i = 0; i < m; ++i)
          d.dy[i] = pvec[i] * d.dtau + qvec[i];
        for (std::size_t j = 0; j < nn; ++j) {
          d.dx[j] = uvec[j] * d.dtau + vx[j];
          d.ds[j] = (rxs[j] - s_[j] * d.dx[j]) / x_[j];
          if (um[j] > 0.5) {
            d.dw[j] = ru_[j] - d.dx[j] + ubar_safe[j] * d.dtau;
            d.dv[j] = (rwv[j] - vm[j] * d.dw[j]) / w_[j];
          }
        }
        d.dkappa = (rtk - kappa_ * d.dtau) / tau_;
        return d;
      };
      auto direction = [&](const Vec &rxs, const Vec &rwv, double rtk, double rho) {
        Vec r1(m), r2(nn), ru_(nn);
        for (std::size_t i = 0; i < m; ++i)
          r1[i] = rp[i] * rho;
        for (std::size_t j = 0; j < nn; ++j) {
          r2[j] = -rd[j] * rho;
          ru_[j] = ru[j] * rho;
        }
        const double r3 = -rg * rho;
        Dir d = reduce(r1, ru_, r2, r3, rxs, rwv, rtk);
        for (int sweep = 0; sweep < opt_.refinement_sweeps; ++sweep) {
          Vec e1(m), e2(nn), eu(nn), e4(nn), e4b(nn);
          Vec adx(m), atdy(nn);
          a_times(d.dx, adx);
          at_times(d.dy, atdy);
          for (std::size_t i = 0; i < m; ++i)
            e1[i] = r1[i] - (adx[i] - S_.b[i] * d.dtau);
          double cdx = 0.0, bdy = 0.0, udv = 0.0;
          for (std::size_t j = 0; j < nn; ++j) {
            const double dvj = um[j] > 0.5 ? d.dv[j] : 0.0;
            eu[j] = um[j] > 0.5 ? ru_[j] - (d.dx[j] + d.dw[j] - ubar_safe[j] * d.dtau) : 0.0;
            e2[j] = r2[j] - (atdy[j] + d.ds[j] - dvj - S_.c[j] * d.dtau);
            e4[j] = rxs[j] - (s_[j] * d.dx[j] + x_[j] * d.ds[j]);
            e4b[j] = um[j] > 0.5 ? rwv[j] - (vm[j] * d.dw[j] + w_[j] * dvj) : 0.0;
            cdx += S_.c[j] * d.dx[j];
            udv += ubar_safe[j] * dvj;
          }
          for (std::size_t i = 0; i < m; ++i)
            bdy += S_.b[i] * d.dy[i];
          const double e3 = r3 - (d.dkappa + cdx - bdy + udv);
          const double e5 = rtk - (kappa_ * d.dtau + tau_ * d.dkappa);
          {
            double en = std::max({amax_abs(e1), amax_abs(e2), amax_abs(eu), amax_abs(e4), amax_abs(e4b), std::abs(e3), std::abs(e5)});
            double rn = std::max({amax_abs(r1), amax_abs(r2), amax_abs(ru_), amax_abs(rxs), amax_abs(rwv), std::abs(r3), std::abs(rtk)}) + 1e-30;
            if (en <= 1e-9 * rn)
              break; // the five Newton equations are already satisfied to working accuracy
          }
          Dir c2 = reduce(e1, eu, e2, e3, e4, e4b, e5);
          for (std::size_t j = 0; j < nn; ++j) {
            d.dx[j] += c2.dx[j]; d.ds[j] += c2.ds[j]; d.dw[j] += c2.dw[j]; d.dv[j] += c2.dv[j];
          }
          for (std::size_t i = 0; i < m; ++i)
            d.dy[i] += c2.dy[i];
          d.dtau += c2.dtau;
          d.dkappa += c2.dkappa;
        }
        return d;
      };
      auto max_step = [&](const Dir &d) {
        double a = 1e30;
        auto ratio = [&](double val, double dv) {
          if (dv < 0.0)
            a = std::min(a, -val / dv);
        };
        for (std::size_t j = 0; j < nn; ++j) {
          ratio(x_[j], d.dx[j]);
          ratio(s_[j], d.ds[j]);
          if (um[j] > 0.5) {
            ratio(w_[j], d.dw[j]);
            ratio(v_[j], d.dv[j]);
          }
        }
        ratio(tau_, d.dtau);
        ratio(kappa_, d.dkappa);
        return a;
      };
      Vec xs_prod(nn), wv_prod(nn, 0.0), rxs(nn), rwv(nn, 0.0);
      double mu_now = tau_ * kappa_;
      for (std::size_t j = 0; j < nn; ++j) {
        xs_prod[j] = x_[j] * s_[j];
        mu_now += xs_prod[j];
        if (um[j] > 0.5) {
          wv_prod[j] = w_[j] * vm[j];
          mu_now += wv_prod[j];
        }
        rxs[j] = -xs_prod[j];
        rwv[j] = -wv_prod[j];
      }
      mu_now /= ncomp;
      Dir aff = direction(rxs, rwv, -tau_ * kappa_, 1.0);
      const double alpha_aff = std::min(1.0, max_step(aff));
      double mu_aff = (tau_ + aff.dtau * alpha_aff) * (kappa_ + aff.dkappa * alpha_aff);
      for (std::size_t j = 0; j < nn; ++j) {
        mu_aff += (x_[j] + aff.dx[j] * alpha_aff) * (s_[j] + aff.ds[j] * alpha_aff);
        if (um[j] > 0.5)
          mu_aff += (w_[j] + aff.dw[j] * alpha_aff) * (vm[j] + aff.dv[j] * alpha_aff);
      }
      mu_aff /= ncomp;
      const double sigma = std::min(1.0, std::pow(mu_aff / mu_now, 3.0));
      const double target = sigma * mu_now;
      for (std::size_t j = 0; j < nn; ++j) {
        rxs[j] = target - xs_prod[j] - aff.dx[j] * aff.ds[j];
        rwv[j] = um[j] > 0.5 ? target - wv_prod[j] - aff.dw[j] * aff.dv[j] : 0.0;
      }
      Dir full = direction(rxs, rwv, target - tau_ * kappa_ - aff.dtau * aff.dkappa, 1.0 - sigma);
      const double alpha = std::min(1.0, 0.9995 * max_step(full));
      for (std::size_t j = 0; j < nn; ++j) {
        x_[j] += alpha * full.dx[j];
        s_[j] += alpha * full.ds[j];
        if (um[j] > 0.5) {
          w_[j] += alpha * full.dw[j];
          v_[j] += alpha * full.dv[j];
        }
      }
      for (std::size_t i = 0; i < m; ++i)
        y_[i] += alpha * full.dy[i];
      tau_ += alpha * full.dtau;
      kappa_ += alpha * full.dkappa;
    }
    if (out.status == SolveStatus::IterationLimit && best_merit < 1e-6) {
      x_ = best_x; s_ = best_s; w_ = best_w; v_ = best_v; y_ = best_y; tau_ = best_tau; kappa_ = best_kappa;
      out.status = SolveStatus::Optimal;
    }
  }

  // matvec with M_raw (regularised-D system, including dense columns): y = A D A^T x
  void m_times_full(const Vec &d, const Vec &x, Vec &y) const {
    Vec t(static_cast<std::size_t>(S_.nn), 0.0);
    at_times(x, t);
    for (std::size_t j = 0; j < t.size(); ++j)
      t[j] *= d[j];
    a_times(t, y);
  }

  // ---------------------------------------------------------------- results in original space
  void extract(IpmSolution &out) {
    const auto n = static_cast<std::size_t>(model_.cols);
    out.x.assign(n, 0.0);
    for (std::size_t j = 0; j < n; ++j) {
      double z = x_[static_cast<std::size_t>(S_.col_pos[j])] / tau_ * col_scale_[static_cast<std::size_t>(S_.col_pos[j])];
      double v = S_.shift[j] + S_.col_sign[j] * z;
      if (S_.col_neg[j] >= 0)
        v -= x_[static_cast<std::size_t>(S_.col_neg[j])] / tau_ * col_scale_[static_cast<std::size_t>(S_.col_neg[j])];
      out.x[j] = v;
    }
    out.row_activity.assign(static_cast<std::size_t>(model_.rows), 0.0);
    model_.a.multiply_add(1.0, out.x, out.row_activity);
    out.row_dual.assign(static_cast<std::size_t>(model_.rows), 0.0);
    for (std::size_t i = 0; i < static_cast<std::size_t>(model_.rows); ++i)
      out.row_dual[i] = y_[i] / tau_ * row_scale_[i];
    double obj = model_.offset;
    for (std::size_t j = 0; j < n; ++j)
      obj += model_.cost[j] * out.x[j];
    out.objective = obj;
  }

  // crossover: crash a basis from the interior point, then let the simplex finish (primal + dual clean-up)
  void crossover(IpmSolution &out) {
    const std::int32_t n = model_.cols, m = model_.rows;
    // distance of every variable (structural, then row logicals) from its nearest bound, relative
    std::vector<std::pair<double, std::int32_t>> cand; // (-distance, var)
    auto dist = [](double v, double lo, double hi) {
      const double dl = std::isinf(lo) ? kInf : v - lo, du = std::isinf(hi) ? kInf : hi - v;
      double d = std::min(dl, du);
      if (std::isinf(d))
        d = 1e30; // free variable: always basic candidate
      return d / (1.0 + std::abs(v));
    };
    for (std::int32_t j = 0; j < n; ++j)
      cand.push_back({-dist(out.x[static_cast<std::size_t>(j)], model_.col_lo[static_cast<std::size_t>(j)], model_.col_hi[static_cast<std::size_t>(j)]), j});
    for (std::int32_t i = 0; i < m; ++i)
      cand.push_back({-dist(out.row_activity[static_cast<std::size_t>(i)], model_.row_lo[static_cast<std::size_t>(i)], model_.row_hi[static_cast<std::size_t>(i)]), n + i});
    std::sort(cand.begin(), cand.end());
    std::vector<VarStatus> basis(static_cast<std::size_t>(n + m), VarStatus::AtLower);
    for (std::int32_t k = 0; k < m; ++k)
      basis[static_cast<std::size_t>(cand[static_cast<std::size_t>(k)].second)] = VarStatus::Basic;
    // nonbasic variables at the nearest bound
    for (std::int32_t j = 0; j < n + m; ++j) {
      if (basis[static_cast<std::size_t>(j)] == VarStatus::Basic)
        continue;
      const bool logical = j >= n;
      const double v = logical ? out.row_activity[static_cast<std::size_t>(j - n)] : out.x[static_cast<std::size_t>(j)];
      const double lo = logical ? model_.row_lo[static_cast<std::size_t>(j - n)] : model_.col_lo[static_cast<std::size_t>(j)];
      const double hi = logical ? model_.row_hi[static_cast<std::size_t>(j - n)] : model_.col_hi[static_cast<std::size_t>(j)];
      const double dl = std::isinf(lo) ? kInf : v - lo, du = std::isinf(hi) ? kInf : hi - v;
      basis[static_cast<std::size_t>(j)] = dl <= du ? VarStatus::AtLower : VarStatus::AtUpper;
      if (std::isinf(lo) && std::isinf(hi))
        basis[static_cast<std::size_t>(j)] = VarStatus::Free;
    }
    SimplexOptions cx_opts = opt_.simplex;
    cx_opts.stop = opt_.stop;
    Simplex sx(model_, cx_opts);
    if (!sx.set_basis(basis))
      return;
    auto sol = sx.solve();
    if (sol.status == SolveStatus::Optimal) {
      out.x = sol.x;
      out.row_activity = sol.row_activity;
      out.row_dual = sol.row_dual;
      out.objective = sol.objective;
      out.crossed_over = true;
      out.crossover_iterations = sol.iterations;
      out.basis = sol.basis;
    }
  }

  Model model_;
  IpmOptions opt_;
  ipmdetail::StdForm S_;
  SparseMatrix at_;
  Vec row_scale_, col_scale_, x_, s_, w_, v_, y_, ray_z_, Mval_, Mraw_;
  double tau_{1.0}, kappa_{1.0};
  std::vector<char> is_dense_;
  std::vector<std::int32_t> dense_cols_;
  std::vector<std::int64_t> Mstart_;
  std::vector<std::int32_t> Midx_;
  std::vector<Vec> minv_u_, small_;
  SparseCholesky chol_;
  bool dense_ready_{false};
};

inline IpmSolution solve_ipm(const Model &model_in, const IpmOptions &opt = {}) {
  Model model = model_in;
  model.canonicalize_infinities(); // sentinel infinities (1e30, 1e300) -> real infinities, see sparse.h kInfThreshold
  InteriorPoint ip(model, opt);
  auto out = ip.solve();
  if (out.status == SolveStatus::Unbounded) {
    // A recession ray with negative cost only proves "unbounded if feasible": an infeasible model can have one too (the
    // self-dual embedding may converge to either certificate). Decide with a feasibility solve (zero objective).
    Model feas = model;
    std::fill(feas.cost.begin(), feas.cost.end(), 0.0);
    feas.offset = 0.0;
    feas.integer.clear();
    SimplexOptions so = opt.simplex;
    so.time_limit_seconds = opt.time_limit_seconds;
    Simplex sx(feas, so);
    const auto f = sx.solve();
    if (f.status == SolveStatus::Infeasible) {
      out.status = SolveStatus::Infeasible;
      out.ray.clear();
      out.farkas = f.farkas;
    }
  }
  return out;
}

} // namespace pluss::subsp::math::lp
