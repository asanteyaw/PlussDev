#pragma once

// LP / MIP presolve with primal postsolve.
//
// Reductions (iterated to a fixed point):
//   empty rows / columns, fixed columns (substituted), singleton rows (-> column bounds; integer bounds rounded), redundant rows and
//   forcing rows from activity bounds, dual fixing / dominated columns from cost signs and constraint locks, bound strengthening of
//   integer columns by activity, duplicate-free coefficient cleanup. Every reduction is exact for the LP and for the MIP.
// Postsolve restores the primal solution of every removed column. Duals and a basis for the ORIGINAL model are obtained by warm
// starting the simplex on the original problem from the expanded basis (removed columns nonbasic at their fixed value, removed rows
// basic), which needs 0 to a few pivots and avoids hand-written dual postsolve for each reduction (see solve.h).

#include "pluss/subsp/math/lp/simplex.h"
#include "pluss/subsp/math/lp/sparse.h"

#include <cmath>
#include <vector>

namespace pluss::subsp::math::lp {

struct PresolveOptions final {
  bool enabled = true;
  double feas_tol = 1e-9;
  int max_passes = 20;
  bool dual_fixing = true;
  bool tighten_integer_bounds = true;
  bool column_singletons = true;      // eliminate implied-free column singletons of equality rows
  bool coefficient_tightening = true; // MIP only: strengthen big-M style coefficients of one-sided rows (Savelsbergh 1994)
};

// A singleton row  lo <= a x_j <= hi  (bounds already shifted by removed columns) that was folded into the bounds of column j.
struct SingletonRow final {
  std::int32_t row, col;
  double coef, lo, hi;
};

// Column x_j that appeared in exactly one (equality) row i and whose bounds are implied by that row: eliminated with the row
// (x_j = (b_i - sum_k a_ik x_k) / a_ij), costs of the other columns of the row adjusted.
struct Substitution final {
  std::int32_t row, col;
  double coef;
};

struct PresolveResult final {
  Model reduced;
  SolveStatus status{SolveStatus::Optimal}; // Infeasible / Unbounded when presolve already proves it
  std::vector<std::int32_t> col_map, row_map;   // reduced index -> original index
  std::vector<double> fixed_value;               // size original cols: value of removed columns (NaN if kept)
  std::vector<char> col_removed, row_removed;
  std::vector<Substitution> substitutions;      // in removal order
  SparseMatrix orig_rows;                       // transpose of the input matrix (row access for the substitution postsolve)
  std::vector<double> orig_row_lo;
  std::vector<SingletonRow> singletons;         // in removal order; used to rebuild the basis of the original model
  std::vector<double> orig_col_lo, orig_col_hi; // bounds of the input model (to place removed columns on the right side)
  std::int32_t original_rows{0}, original_cols{0};
  double objective_offset{0.0};
  int removed_rows{0}, removed_cols{0}, passes{0};

  // x of the reduced model -> x of the original model
  [[nodiscard]] std::vector<double> expand(const std::vector<double> &xr) const {
    std::vector<double> x(static_cast<std::size_t>(original_cols), 0.0);
    for (std::int32_t j = 0; j < original_cols; ++j)
      if (col_removed[static_cast<std::size_t>(j)])
        x[static_cast<std::size_t>(j)] = fixed_value[static_cast<std::size_t>(j)];
    for (std::size_t k = 0; k < col_map.size(); ++k)
      x[static_cast<std::size_t>(col_map[k])] = xr[k];
    for (std::size_t t = substitutions.size(); t-- > 0;) { // latest elimination first: its row only holds columns that are known by now
      const auto &sb = substitutions[t];
      const auto i = static_cast<std::size_t>(sb.row);
      double rest = 0.0;
      for (std::int64_t q = orig_rows.start[i]; q < orig_rows.start[i + 1]; ++q) {
        const auto k = static_cast<std::size_t>(orig_rows.index[static_cast<std::size_t>(q)]);
        if (k != static_cast<std::size_t>(sb.col))
          rest += orig_rows.value[static_cast<std::size_t>(q)] * x[k];
      }
      x[static_cast<std::size_t>(sb.col)] = (orig_row_lo[i] - rest) / sb.coef;
    }
    return x;
  }
  // basis of the reduced model -> crash basis of the original model (size cols + rows). `xr` is the reduced primal solution: a column
  // that sits at a bound created by a singleton row becomes basic and that row's logical becomes non-basic (the row is tight), which
  // gives the removed row its dual value and keeps the basis dual feasible; without it the expanded basis is far from optimal.
  [[nodiscard]] std::vector<VarStatus> expand_basis(const std::vector<VarStatus> &br, const std::vector<double> &xr = {}) const {
    const auto n0 = static_cast<std::size_t>(original_cols), m0 = static_cast<std::size_t>(original_rows);
    std::vector<VarStatus> b(n0 + m0, VarStatus::AtLower);
    for (std::size_t i = 0; i < m0; ++i)
      b[n0 + i] = VarStatus::Basic; // removed rows: logical basic
    for (std::size_t j = 0; j < n0; ++j) // removed columns sit at the original bound they were fixed to
      if (col_removed[j] && !orig_col_lo.empty()) {
        const double v = fixed_value[j];
        const double dl = std::isinf(orig_col_lo[j]) ? kInf : std::abs(v - orig_col_lo[j]);
        const double du = std::isinf(orig_col_hi[j]) ? kInf : std::abs(v - orig_col_hi[j]);
        if (std::isinf(dl) && std::isinf(du))
          b[j] = VarStatus::Free;
        else
          b[j] = du < dl ? VarStatus::AtUpper : VarStatus::AtLower;
      }
    const std::size_t nr = col_map.size();
    for (std::size_t k = 0; k < nr; ++k)
      b[static_cast<std::size_t>(col_map[k])] = br[k];
    for (std::size_t i = 0; i < row_map.size(); ++i)
      b[n0 + static_cast<std::size_t>(row_map[i])] = br[nr + i];
    // singleton rows, latest first: the row that created the binding bound takes over from its column
    std::vector<std::size_t> reduced_index(n0, std::numeric_limits<std::size_t>::max());
    for (std::size_t k = 0; k < nr; ++k)
      reduced_index[static_cast<std::size_t>(col_map[k])] = k;
    for (std::size_t t = singletons.size(); t-- > 0;) {
      const auto &sr = singletons[t];
      const auto j = static_cast<std::size_t>(sr.col);
      const auto i = static_cast<std::size_t>(sr.row);
      if (b[n0 + i] != VarStatus::Basic || b[j] == VarStatus::Basic)
        continue;
      double v;
      if (col_removed[j])
        v = fixed_value[j];
      else if (reduced_index[j] < xr.size())
        v = xr[reduced_index[j]];
      else
        continue;
      const double act = sr.coef * v;
      const double scale = 1.0 + std::abs(act);
      const bool at_lo = !std::isinf(sr.lo) && std::abs(act - sr.lo) <= 1e-9 * scale;
      const bool at_hi = !std::isinf(sr.hi) && std::abs(act - sr.hi) <= 1e-9 * scale;
      if (!at_lo && !at_hi)
        continue;
      // the column must be nonbasic at exactly the value this row pins (not at an original bound that happens to coincide)
      b[j] = VarStatus::Basic;
      b[n0 + i] = at_lo ? VarStatus::AtLower : VarStatus::AtUpper;
    }
    for (const auto &sb : substitutions) { // eliminated column is basic, its (equality) row non-basic: row dual = c_j / a_ij
      b[static_cast<std::size_t>(sb.col)] = VarStatus::Basic;
      b[n0 + static_cast<std::size_t>(sb.row)] = VarStatus::AtLower;
    }
    return b;
  }
};

inline PresolveResult presolve(const Model &in, const PresolveOptions &opt = {}) {
  in.validate();
  PresolveResult r;
  r.original_rows = in.rows;
  r.original_cols = in.cols;
  const auto m = static_cast<std::size_t>(in.rows), n = static_cast<std::size_t>(in.cols);
  r.col_removed.assign(n, 0);
  r.row_removed.assign(m, 0);
  r.fixed_value.assign(n, std::numeric_limits<double>::quiet_NaN());
  Model w = in; // working copy (bounds are tightened in place)
  r.orig_col_lo = in.col_lo;
  r.orig_row_lo = in.row_lo;
  r.orig_col_hi = in.col_hi;
  const bool has_int = !in.integer.empty();
  auto is_int = [&](std::size_t j) { return has_int && in.integer[j]; };
  const SparseMatrix &a = in.a;
  const SparseMatrix at = a.transposed(); // rows as "columns"
  r.orig_rows = at;
  std::vector<std::int32_t> row_cnt(m, 0), col_cnt(n, 0);
  for (std::size_t j = 0; j < n; ++j)
    col_cnt[j] = static_cast<std::int32_t>(a.start[j + 1] - a.start[j]);
  for (std::size_t i = 0; i < m; ++i)
    row_cnt[i] = static_cast<std::int32_t>(at.start[i + 1] - at.start[i]);
  const double tol = opt.feas_tol;
  auto fail = [&](SolveStatus s) {
    r.status = s;
    return r;
  };
  auto round_int_bounds = [&](std::size_t j) {
    if (is_int(j)) {
      if (!std::isinf(w.col_lo[j]))
        w.col_lo[j] = std::ceil(w.col_lo[j] - 1e-6);
      if (!std::isinf(w.col_hi[j]))
        w.col_hi[j] = std::floor(w.col_hi[j] + 1e-6);
    }
  };
  for (std::size_t j = 0; j < n; ++j)
    round_int_bounds(j);

  auto remove_column = [&](std::size_t j, double value) {
    r.col_removed[j] = 1;
    r.fixed_value[j] = value;
    ++r.removed_cols;
    w.offset += w.cost[j] * value;
    for (std::int64_t p = a.start[j]; p < a.start[j + 1]; ++p) {
      const auto i = static_cast<std::size_t>(a.index[static_cast<std::size_t>(p)]);
      if (r.row_removed[i])
        continue;
      const double shift = a.value[static_cast<std::size_t>(p)] * value;
      if (!std::isinf(w.row_lo[i]))
        w.row_lo[i] -= shift;
      if (!std::isinf(w.row_hi[i]))
        w.row_hi[i] -= shift;
      --row_cnt[i];
    }
  };
  auto remove_row = [&](std::size_t i) {
    r.row_removed[i] = 1;
    ++r.removed_rows;
    for (std::int64_t p = at.start[i]; p < at.start[i + 1]; ++p) {
      const auto j = static_cast<std::size_t>(at.index[static_cast<std::size_t>(p)]);
      if (!r.col_removed[j])
        --col_cnt[j];
    }
  };

  for (int pass = 0; pass < opt.max_passes; ++pass) {
    bool changed = false;
    ++r.passes;
    // ---- fixed columns, empty columns
    for (std::size_t j = 0; j < n; ++j) {
      if (r.col_removed[j])
        continue;
      if (w.col_lo[j] > w.col_hi[j] + tol)
        return fail(SolveStatus::Infeasible);
      if (w.col_hi[j] - w.col_lo[j] <= 1e-12 && !std::isinf(w.col_lo[j])) {
        remove_column(j, w.col_lo[j]);
        changed = true;
        continue;
      }
      if (col_cnt[j] == 0) {
        const double c = w.cost[j];
        double v;
        if (c > 0)
          v = w.col_lo[j];
        else if (c < 0)
          v = w.col_hi[j];
        else
          v = !std::isinf(w.col_lo[j]) ? w.col_lo[j] : (!std::isinf(w.col_hi[j]) ? w.col_hi[j] : 0.0);
        if (std::isinf(v))
          return fail(SolveStatus::Unbounded);
        remove_column(j, v);
        changed = true;
      }
    }
    // ---- rows
    for (std::size_t i = 0; i < m; ++i) {
      if (r.row_removed[i])
        continue;
      if (w.row_lo[i] > w.row_hi[i] + tol)
        return fail(SolveStatus::Infeasible);
      if (row_cnt[i] == 0) {
        if (w.row_lo[i] > tol || w.row_hi[i] < -tol)
          return fail(SolveStatus::Infeasible);
        remove_row(i);
        changed = true;
        continue;
      }
      if (row_cnt[i] == 1) { // singleton row -> bounds on its column
        std::size_t jj = n;
        double aij = 0.0;
        for (std::int64_t p = at.start[i]; p < at.start[i + 1]; ++p) {
          const auto j = static_cast<std::size_t>(at.index[static_cast<std::size_t>(p)]);
          if (!r.col_removed[j]) {
            jj = j;
            aij = at.value[static_cast<std::size_t>(p)];
          }
        }
        if (jj < n && std::abs(aij) > 1e-12) {
          double lo = w.row_lo[i] / aij, hi = w.row_hi[i] / aij;
          if (aij < 0)
            std::swap(lo, hi);
          if (std::isnan(lo))
            lo = -kInf;
          if (std::isnan(hi))
            hi = kInf;
          w.col_lo[jj] = std::max(w.col_lo[jj], lo);
          w.col_hi[jj] = std::min(w.col_hi[jj], hi);
          round_int_bounds(jj);
          if (w.col_lo[jj] > w.col_hi[jj] + tol)
            return fail(SolveStatus::Infeasible);
          r.singletons.push_back({static_cast<std::int32_t>(i), static_cast<std::int32_t>(jj), aij, w.row_lo[i], w.row_hi[i]});
          remove_row(i);
          changed = true;
          continue;
        }
      }
      // activity bounds with counters of infinite contributions
      double amin = 0.0, amax = 0.0;
      int min_inf = 0, max_inf = 0;
      for (std::int64_t p = at.start[i]; p < at.start[i + 1]; ++p) {
        const auto j = static_cast<std::size_t>(at.index[static_cast<std::size_t>(p)]);
        if (r.col_removed[j])
          continue;
        const double v = at.value[static_cast<std::size_t>(p)];
        const double lo = w.col_lo[j], hi = w.col_hi[j];
        if (v > 0) {
          if (std::isinf(lo)) ++min_inf; else amin += v * lo;
          if (std::isinf(hi)) ++max_inf; else amax += v * hi;
        } else {
          if (std::isinf(hi)) ++min_inf; else amin += v * hi;
          if (std::isinf(lo)) ++max_inf; else amax += v * lo;
        }
      }
      const double scale = 1.0 + std::abs(amin) + std::abs(amax);
      const bool lo_fin = !std::isinf(w.row_lo[i]), hi_fin = !std::isinf(w.row_hi[i]);
      if ((hi_fin && min_inf == 0 && amin > w.row_hi[i] + tol * scale) || (lo_fin && max_inf == 0 && amax < w.row_lo[i] - tol * scale))
        return fail(SolveStatus::Infeasible);
      const bool lo_ok = !lo_fin || (min_inf == 0 && amin >= w.row_lo[i] - tol * scale);
      const bool hi_ok = !hi_fin || (max_inf == 0 && amax <= w.row_hi[i] + tol * scale);
      if (lo_ok && hi_ok) { // redundant row
        remove_row(i);
        changed = true;
        continue;
      }
      // forcing rows: minimum (maximum) activity equals the upper (lower) side, so every column sits at the bound giving it
      if (hi_fin && min_inf == 0 && std::abs(amin - w.row_hi[i]) <= tol * scale) {
        for (std::int64_t p = at.start[i]; p < at.start[i + 1]; ++p) {
          const auto j = static_cast<std::size_t>(at.index[static_cast<std::size_t>(p)]);
          if (r.col_removed[j])
            continue;
          if (at.value[static_cast<std::size_t>(p)] > 0) w.col_hi[j] = w.col_lo[j]; else w.col_lo[j] = w.col_hi[j];
        }
        remove_row(i);
        changed = true;
        continue;
      }
      if (lo_fin && max_inf == 0 && std::abs(amax - w.row_lo[i]) <= tol * scale) {
        for (std::int64_t p = at.start[i]; p < at.start[i + 1]; ++p) {
          const auto j = static_cast<std::size_t>(at.index[static_cast<std::size_t>(p)]);
          if (r.col_removed[j])
            continue;
          if (at.value[static_cast<std::size_t>(p)] > 0) w.col_lo[j] = w.col_hi[j]; else w.col_hi[j] = w.col_lo[j];
        }
        remove_row(i);
        changed = true;
        continue;
      }
      // integer bound strengthening (continuous columns are left alone: it can erode feasibility tolerances)
      if (opt.tighten_integer_bounds && has_int && row_cnt[i] <= 200) {
        for (std::int64_t p = at.start[i]; p < at.start[i + 1]; ++p) {
          const auto k = static_cast<std::size_t>(at.index[static_cast<std::size_t>(p)]);
          if (r.col_removed[k] || !is_int(k))
            continue;
          const double v = at.value[static_cast<std::size_t>(p)];
          if (std::abs(v) < 1e-9)
            continue;
          const double lo = w.col_lo[k], hi = w.col_hi[k];
          // contribution of k to the minimum / maximum activity (and whether it is infinite)
          const bool kmin_inf = v > 0 ? std::isinf(lo) : std::isinf(hi), kmax_inf = v > 0 ? std::isinf(hi) : std::isinf(lo);
          const double kmin = kmin_inf ? 0.0 : v * (v > 0 ? lo : hi), kmax = kmax_inf ? 0.0 : v * (v > 0 ? hi : lo);
          if (hi_fin && min_inf - (kmin_inf ? 1 : 0) == 0) { // a_k x_k <= row_hi - (min activity of the others)
            const double rest = w.row_hi[i] - (amin - kmin);
            if (v > 0) {
              const double nh = std::floor(rest / v + 1e-6);
              if (nh < w.col_hi[k] - 0.5) { w.col_hi[k] = nh; changed = true; }
            } else {
              const double nl = std::ceil(rest / v - 1e-6);
              if (nl > w.col_lo[k] + 0.5) { w.col_lo[k] = nl; changed = true; }
            }
          }
          if (lo_fin && max_inf - (kmax_inf ? 1 : 0) == 0) { // a_k x_k >= row_lo - (max activity of the others)
            const double rest = w.row_lo[i] - (amax - kmax);
            if (v > 0) {
              const double nl = std::ceil(rest / v - 1e-6);
              if (nl > w.col_lo[k] + 0.5) { w.col_lo[k] = nl; changed = true; }
            } else {
              const double nh = std::floor(rest / v + 1e-6);
              if (nh < w.col_hi[k] - 0.5) { w.col_hi[k] = nh; changed = true; }
            }
          }
          if (w.col_lo[k] > w.col_hi[k] + 1e-9)
            return fail(SolveStatus::Infeasible);
        }
      }
    }
    // ---- column singletons in equality rows with implied-free bounds: eliminate column and row (no fill-in)
    if (opt.column_singletons) {
      for (std::size_t j = 0; j < n; ++j) {
        if (r.col_removed[j] || col_cnt[j] != 1 || is_int(j))
          continue;
        std::size_t i = m;
        double aij = 0.0;
        for (std::int64_t p = a.start[j]; p < a.start[j + 1]; ++p) {
          const auto ii = static_cast<std::size_t>(a.index[static_cast<std::size_t>(p)]);
          if (!r.row_removed[ii]) {
            i = ii;
            aij = a.value[static_cast<std::size_t>(p)];
          }
        }
        if (i >= m || std::abs(aij) < 1e-9 || std::isinf(w.row_lo[i]) || w.row_lo[i] != w.row_hi[i] || row_cnt[i] < 2 || row_cnt[i] > 5000)
          continue;
        double amin = 0.0, amax = 0.0, amax_abs = 0.0;
        int min_inf = 0, max_inf = 0;
        for (std::int64_t q = at.start[i]; q < at.start[i + 1]; ++q) {
          const auto k = static_cast<std::size_t>(at.index[static_cast<std::size_t>(q)]);
          if (r.col_removed[k] || k == j)
            continue;
          const double v = at.value[static_cast<std::size_t>(q)];
          amax_abs = std::max(amax_abs, std::abs(v));
          const double lo = w.col_lo[k], hi = w.col_hi[k];
          if (v > 0) {
            if (std::isinf(lo)) ++min_inf; else amin += v * lo;
            if (std::isinf(hi)) ++max_inf; else amax += v * hi;
          } else {
            if (std::isinf(hi)) ++min_inf; else amin += v * hi;
            if (std::isinf(lo)) ++max_inf; else amax += v * lo;
          }
        }
        if (amax_abs > 1e3 * std::abs(aij))
          continue; // the elimination would amplify the other coefficients in the costs
        // implied range of x_j = (b - activity) / aij
        double ilo = -kInf, ihi = kInf;
        const double b = w.row_lo[i];
        const double e_lo = max_inf == 0 ? (b - amax) / aij : (aij > 0 ? -kInf : kInf);
        const double e_hi = min_inf == 0 ? (b - amin) / aij : (aij > 0 ? kInf : -kInf);
        if (aij > 0) { ilo = e_lo; ihi = e_hi; } else { ilo = e_hi; ihi = e_lo; }
        const double ftol = 1e-9 * (1.0 + std::abs(b));
        if ((!std::isinf(w.col_lo[j]) && !(ilo >= w.col_lo[j] - ftol)) || (!std::isinf(w.col_hi[j]) && !(ihi <= w.col_hi[j] + ftol)))
          continue; // not implied free
        // eliminate: costs of the row's other columns absorb c_j
        const double cj = w.cost[j];
        if (cj != 0.0) {
          for (std::int64_t q = at.start[i]; q < at.start[i + 1]; ++q) {
            const auto k = static_cast<std::size_t>(at.index[static_cast<std::size_t>(q)]);
            if (r.col_removed[k] || k == j)
              continue;
            w.cost[k] -= cj * at.value[static_cast<std::size_t>(q)] / aij;
          }
          w.offset += cj * b / aij;
        }
        r.col_removed[j] = 1;
        r.fixed_value[j] = 0.0; // recomputed by expand() from the row
        ++r.removed_cols;
        for (std::int64_t p = a.start[j]; p < a.start[j + 1]; ++p) { // j's single live row is i: nothing else to shift
        }
        r.substitutions.push_back({static_cast<std::int32_t>(i), static_cast<std::int32_t>(j), aij});
        remove_row(i);
        changed = true;
      }
    }
    // ---- dual fixing (locks)
    if (opt.dual_fixing) {
      for (std::size_t j = 0; j < n; ++j) {
        if (r.col_removed[j])
          continue;
        if (w.col_lo[j] > w.col_hi[j] + tol)
          return fail(SolveStatus::Infeasible);
        bool uplock = false, downlock = false;
        for (std::int64_t p = a.start[j]; p < a.start[j + 1]; ++p) {
          const auto i = static_cast<std::size_t>(a.index[static_cast<std::size_t>(p)]);
          if (r.row_removed[i])
            continue;
          const double v = a.value[static_cast<std::size_t>(p)];
          const bool has_lo = !std::isinf(w.row_lo[i]), has_hi = !std::isinf(w.row_hi[i]);
          if (v > 0) {
            if (has_hi) uplock = true;
            if (has_lo) downlock = true;
          } else {
            if (has_lo) uplock = true;
            if (has_hi) downlock = true;
          }
        }
        const double c = w.cost[j];
        if (c >= 0 && !downlock && !std::isinf(w.col_lo[j])) {
          w.col_hi[j] = w.col_lo[j];
          changed = true;
        } else if (c <= 0 && !uplock && !std::isinf(w.col_hi[j])) {
          w.col_lo[j] = w.col_hi[j];
          changed = true;
        } else if (c > 0 && !downlock && std::isinf(w.col_lo[j])) {
          return fail(SolveStatus::InfeasibleOrUnbounded);
        } else if (c < 0 && !uplock && std::isinf(w.col_hi[j])) {
          return fail(SolveStatus::InfeasibleOrUnbounded);
        }
      }
    }
    if (!changed)
      break;
  }

  // ---- build the reduced model
  std::vector<std::int32_t> new_col(n, -1), new_row(m, -1);
  for (std::size_t j = 0; j < n; ++j)
    if (!r.col_removed[j]) {
      new_col[j] = static_cast<std::int32_t>(r.col_map.size());
      r.col_map.push_back(static_cast<std::int32_t>(j));
    }
  for (std::size_t i = 0; i < m; ++i)
    if (!r.row_removed[i]) {
      new_row[i] = static_cast<std::int32_t>(r.row_map.size());
      r.row_map.push_back(static_cast<std::int32_t>(i));
    }
  Model &red = r.reduced;
  red.rows = static_cast<std::int32_t>(r.row_map.size());
  red.cols = static_cast<std::int32_t>(r.col_map.size());
  red.offset = w.offset;
  for (const auto j : r.col_map) {
    red.cost.push_back(w.cost[static_cast<std::size_t>(j)]);
    red.col_lo.push_back(w.col_lo[static_cast<std::size_t>(j)]);
    red.col_hi.push_back(w.col_hi[static_cast<std::size_t>(j)]);
    if (has_int)
      red.integer.push_back(in.integer[static_cast<std::size_t>(j)]);
    if (!in.col_names.empty())
      red.col_names.push_back(in.col_names[static_cast<std::size_t>(j)]);
  }
  for (const auto i : r.row_map) {
    red.row_lo.push_back(w.row_lo[static_cast<std::size_t>(i)]);
    red.row_hi.push_back(w.row_hi[static_cast<std::size_t>(i)]);
    if (!in.row_names.empty())
      red.row_names.push_back(in.row_names[static_cast<std::size_t>(i)]);
  }
  std::vector<std::int32_t> tr, tc;
  std::vector<double> tv;
  for (std::size_t j = 0; j < n; ++j) {
    if (r.col_removed[j])
      continue;
    for (std::int64_t p = a.start[j]; p < a.start[j + 1]; ++p) {
      const auto i = static_cast<std::size_t>(a.index[static_cast<std::size_t>(p)]);
      if (r.row_removed[i])
        continue;
      tr.push_back(new_row[i]);
      tc.push_back(new_col[j]);
      tv.push_back(a.value[static_cast<std::size_t>(p)]);
    }
  }
  red.a = SparseMatrix::from_triplets(red.rows, red.cols, tr, tc, tv);
  // ---- coefficient tightening on the reduced model (valid for the integer hull, strengthens the LP relaxation)
  if (opt.coefficient_tightening && has_int && red.rows > 0) {
    SparseMatrix csr = red.a.transposed(); // rows
    bool any = false;
    // implied bounds (not written back to the model): propagate row activities over a few passes
    std::vector<double> ilo = red.col_lo, ihi = red.col_hi;
    for (int pass = 0; pass < 3; ++pass) {
      for (std::int32_t i = 0; i < red.rows; ++i) {
        const auto si = static_cast<std::size_t>(i);
        const auto beg = csr.start[si], end = csr.start[si + 1];
        if (end - beg > 500)
          continue;
        double amin = 0.0, amax = 0.0;
        int min_inf = 0, max_inf = 0;
        for (auto q = beg; q < end; ++q) {
          const auto j = static_cast<std::size_t>(csr.index[static_cast<std::size_t>(q)]);
          const double v = csr.value[static_cast<std::size_t>(q)];
          if (v > 0) {
            if (std::isinf(ilo[j])) ++min_inf; else amin += v * ilo[j];
            if (std::isinf(ihi[j])) ++max_inf; else amax += v * ihi[j];
          } else {
            if (std::isinf(ihi[j])) ++min_inf; else amin += v * ihi[j];
            if (std::isinf(ilo[j])) ++max_inf; else amax += v * ilo[j];
          }
        }
        for (auto q = beg; q < end; ++q) {
          const auto j = static_cast<std::size_t>(csr.index[static_cast<std::size_t>(q)]);
          const double v = csr.value[static_cast<std::size_t>(q)];
          if (std::abs(v) < 1e-9)
            continue;
          // row_hi bound: v x_j <= row_hi - (min activity of the others)
          if (!std::isinf(red.row_hi[si])) {
            const bool own_inf = v > 0 ? std::isinf(ilo[j]) : std::isinf(ihi[j]);
            if (min_inf - (own_inf ? 1 : 0) == 0) {
              const double own = own_inf ? 0.0 : v * (v > 0 ? ilo[j] : ihi[j]);
              const double rest = red.row_hi[si] - (amin - own);
              if (v > 0) ihi[j] = std::min(ihi[j], rest / v); else ilo[j] = std::max(ilo[j], rest / v);
            }
          }
          if (!std::isinf(red.row_lo[si])) {
            const bool own_inf = v > 0 ? std::isinf(ihi[j]) : std::isinf(ilo[j]);
            if (max_inf - (own_inf ? 1 : 0) == 0) {
              const double own = own_inf ? 0.0 : v * (v > 0 ? ihi[j] : ilo[j]);
              const double rest = red.row_lo[si] - (amax - own);
              if (v > 0) ilo[j] = std::max(ilo[j], rest / v); else ihi[j] = std::min(ihi[j], rest / v);
            }
          }
        }
      }
    }
    for (int pass = 0; pass < 5; ++pass) {
      bool changed = false;
      for (std::int32_t i = 0; i < red.rows; ++i) {
        const auto si = static_cast<std::size_t>(i);
        const bool lo_fin = !std::isinf(red.row_lo[si]), hi_fin = !std::isinf(red.row_hi[si]);
        if (lo_fin == hi_fin)
          continue; // equality / ranged / free rows are left alone
        const double sgn = hi_fin ? 1.0 : -1.0;       // work with  sum (sgn a) x <= sgn * bound
        double bnd = hi_fin ? red.row_hi[si] : -red.row_lo[si];
        const auto beg = csr.start[si], end = csr.start[si + 1];
        double maxact = 0.0;
        bool finite = true;
        for (auto q = beg; q < end && finite; ++q) {
          const auto j = static_cast<std::size_t>(csr.index[static_cast<std::size_t>(q)]);
          const double a = sgn * csr.value[static_cast<std::size_t>(q)];
          const double lo = ilo[j], hi = ihi[j]; // implied bounds (valid for every feasible point)
          if (a > 0) { if (std::isinf(hi)) finite = false; else maxact += a * hi; }
          else { if (std::isinf(lo)) finite = false; else maxact += a * lo; }
        }
        if (!finite)
          continue;
        for (auto q = beg; q < end; ++q) {
          const auto j = static_cast<std::size_t>(csr.index[static_cast<std::size_t>(q)]);
          if (!red.integer[j])
            continue;
          const double lo = red.col_lo[j], hi = red.col_hi[j];
          if (hi - lo != 1.0)
            continue; // binary-type ranges only (unit range)
          const double a = sgn * csr.value[static_cast<std::size_t>(q)];
          if (a > 1e-9 && ihi[j] != hi) // the activity bound above used a tighter implied bound than the column's own: formulas below assume equality
            continue;
          if (a < -1e-9 && ilo[j] != lo)
            continue;
          if (a > 1e-9 && maxact - a < bnd - 1e-9) {          // row redundant when x_j sits at its lower bound
            const double d = bnd - (maxact - a);
            const double na = a - d;
            // x_j in [lo, lo+1]: shift handled by bound arithmetic  a' = a - d, bound' = bound - d * hi
            csr.value[static_cast<std::size_t>(q)] = sgn * na;
            bnd -= d * hi;
            maxact -= d * hi;
            changed = true;
            any = true;
          } else if (a < -1e-9 && maxact + a < bnd - 1e-9) {  // row redundant when x_j sits at its upper bound
            const double d = bnd - (maxact + a);
            csr.value[static_cast<std::size_t>(q)] = sgn * (a + d);
            bnd += d * lo; // x_j = lo + y: keep the row unchanged at x_j = lo
            maxact += d * lo;
            changed = true;
            any = true;
          }
        }
        if (hi_fin)
          red.row_hi[si] = bnd;
        else
          red.row_lo[si] = -bnd;
      }
      if (!changed)
        break;
    }
    if (any)
      red.a = csr.transposed(); // back to column-wise
  }
  r.objective_offset = w.offset;
  return r;
}

} // namespace pluss::subsp::math::lp
