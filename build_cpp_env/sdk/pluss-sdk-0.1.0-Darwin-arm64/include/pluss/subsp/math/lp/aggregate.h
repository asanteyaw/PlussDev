#pragma once

// Second presolve stage for LPs: column elimination through equality rows, on a DYNAMIC copy of the matrix.
//
//   a_j x_j + sum_k a_k x_k = c   (an equality row)   =>   x_j = (c - sum_k a_k x_k) / a_j
//
// x_j is substituted into every other row it appears in (their coefficients of the x_k absorb -a_ij a_k / a_j, their bounds shift by
// a_ij c / a_j), its cost moves onto the x_k, and the column and the row disappear. Two cases are eliminated:
//   * doubleton equations (two entries): the bounds of x_j become bounds of x_k, so x_j need not be free;
//   * rows of up to `max_row_length` entries in which x_j is IMPLIED FREE (the row and the bounds of the other columns already keep
//     x_j inside its own bounds), the "free column substitution / aggregator" of Andersen & Andersen.
// Fill-in is the reason this stage works on its own mutable row / column lists instead of the static matrix of presolve.h; that
// presolve stays untouched (it is the verified, MILP-safe one) and this stage is only used by the LP entry point, on the output of
// presolve.h, repeated until nothing changes.
//
// Postsolve: x_j is recomputed from the row; the basis gets x_j basic and the equality row's logical non-basic (the elimination is a
// pivot on a_ij, so the expanded basis is non-singular exactly when the reduced one is).
//
// References: Andersen & Andersen, "Presolving in linear programming", Math. Programming 71 (1995); Gondzio, "Presolve analysis of linear
// programs prior to applying an interior point method", INFORMS J. Computing 9 (1997).

#include "pluss/subsp/math/lp/simplex.h"
#include "pluss/subsp/math/lp/sparse.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace pluss::subsp::math::lp {

struct AggregateOptions final {
  bool enabled = true;
  double max_coefficient_ratio = 10.0; // eliminate x_j only if |a_j| * ratio >= max |a_k| of its row (well-conditioned pivot)
  std::int32_t max_row_length = 40;    // longest equality row used for an implied-free substitution
  std::int32_t max_fill = 30;          // skip when eliminating x_j would touch more than this many other rows
  std::int32_t extra_fill = 10;        // allowed growth of the non-zero count beyond what the removed row and column free
};

struct AggregateResult final {
  Model reduced;
  SolveStatus status{SolveStatus::Optimal}; // Infeasible when a substitution produced contradictory bounds
  std::vector<std::int32_t> col_map, row_map;
  struct Record final {
    std::int32_t row, col;
    double a, c;                   // coefficient of x_col in the row, right-hand side
    std::int64_t pool_start{0};    // the row's other entries (column, coefficient): x_col = (c - sum coef x) / a
    std::int32_t pool_len{0};
    // doubleton only (pool_len == 1): bounds of the kept column before this elimination tightened them, bounds of x_col
    double other_lo{0.0}, other_hi{0.0}, col_lo{0.0}, col_hi{0.0};
  };
  std::vector<Record> stack;
  std::vector<std::int32_t> pool_col;
  std::vector<double> pool_val;
  std::int32_t original_rows{0}, original_cols{0};

  [[nodiscard]] bool changed() const { return !stack.empty(); }

  [[nodiscard]] std::vector<double> expand(const std::vector<double> &xr) const {
    std::vector<double> x(static_cast<std::size_t>(original_cols), 0.0);
    for (std::size_t k = 0; k < col_map.size(); ++k)
      x[static_cast<std::size_t>(col_map[k])] = xr[k];
    for (std::size_t t = stack.size(); t-- > 0;) { // latest elimination first: the row's other columns are kept or eliminated later
      const auto &r = stack[t];
      double rest = 0.0;
      for (std::int32_t q = 0; q < r.pool_len; ++q)
        rest += pool_val[static_cast<std::size_t>(r.pool_start) + static_cast<std::size_t>(q)] * x[static_cast<std::size_t>(pool_col[static_cast<std::size_t>(r.pool_start) + static_cast<std::size_t>(q)])];
      x[static_cast<std::size_t>(r.col)] = (r.c - rest) / r.a;
    }
    return x;
  }

  // `xr` is the reduced solution (kept columns): after a doubleton elimination a kept column sitting at a bound that was inherited from
  // the eliminated column's bound is not at a bound of the original model, so it becomes basic and the eliminated column takes that
  // bound instead.
  [[nodiscard]] std::vector<VarStatus> expand_basis(const std::vector<VarStatus> &br, const std::vector<double> &xr = {}) const {
    const auto n0 = static_cast<std::size_t>(original_cols), m0 = static_cast<std::size_t>(original_rows);
    std::vector<VarStatus> b(n0 + m0, VarStatus::AtLower);
    const std::size_t nr = col_map.size();
    for (std::size_t k = 0; k < nr; ++k)
      b[static_cast<std::size_t>(col_map[k])] = br[k];
    for (std::size_t i = 0; i < row_map.size(); ++i)
      b[n0 + static_cast<std::size_t>(row_map[i])] = br[nr + i];
    const std::vector<double> x = xr.empty() ? std::vector<double>() : expand(xr);
    for (std::size_t t = stack.size(); t-- > 0;) {
      const auto &r = stack[t];
      const auto j = static_cast<std::size_t>(r.col);
      b[n0 + static_cast<std::size_t>(r.row)] = VarStatus::AtLower; // equality row: logical non-basic
      b[j] = VarStatus::Basic;
      if (r.pool_len != 1 || x.empty())
        continue;
      const auto k = static_cast<std::size_t>(pool_col[static_cast<std::size_t>(r.pool_start)]);
      if (b[k] == VarStatus::Basic || b[k] == VarStatus::Free)
        continue;
      const double xk = x[k], xj = x[j];
      const double scale = 1.0 + std::abs(xk);
      const bool at_inherited = (b[k] == VarStatus::AtLower && std::abs(xk - (std::isinf(r.other_lo) ? -kInf : r.other_lo)) > 1e-9 * scale) ||
                                (b[k] == VarStatus::AtUpper && std::abs(xk - (std::isinf(r.other_hi) ? kInf : r.other_hi)) > 1e-9 * scale);
      if (!at_inherited)
        continue;
      // the bound that pins x_k comes from x_col: swap their roles
      const double dl = std::isinf(r.col_lo) ? kInf : std::abs(xj - r.col_lo), du = std::isinf(r.col_hi) ? kInf : std::abs(xj - r.col_hi);
      if (std::isinf(dl) && std::isinf(du))
        continue;
      b[k] = VarStatus::Basic;
      b[j] = du < dl ? VarStatus::AtUpper : VarStatus::AtLower;
    }
    return b;
  }
};

inline AggregateResult aggregate(const Model &in, const AggregateOptions &opt = {}) {
  in.validate();
  AggregateResult res;
  res.original_rows = in.rows;
  res.original_cols = in.cols;
  const auto m = static_cast<std::size_t>(in.rows), n = static_cast<std::size_t>(in.cols);
  const auto identity = [&]() {
    res.reduced = in;
    res.reduced.integer.clear();
    for (std::int32_t j = 0; j < in.cols; ++j)
      res.col_map.push_back(j);
    for (std::int32_t i = 0; i < in.rows; ++i)
      res.row_map.push_back(i);
  };
  if (!opt.enabled || m == 0 || n == 0) {
    identity();
    return res;
  }
  // dynamic structure: values live in the rows, columns only list their rows
  struct Entry {
    std::int32_t col;
    double val;
  };
  std::vector<std::vector<Entry>> rows(m);
  std::vector<std::vector<std::int32_t>> cols(n);
  {
    const SparseMatrix at = in.a.transposed();
    for (std::size_t i = 0; i < m; ++i)
      for (auto p = at.start[i]; p < at.start[i + 1]; ++p)
        rows[i].push_back({at.index[static_cast<std::size_t>(p)], at.value[static_cast<std::size_t>(p)]});
    for (std::size_t j = 0; j < n; ++j)
      for (auto p = in.a.start[j]; p < in.a.start[j + 1]; ++p)
        cols[j].push_back(in.a.index[static_cast<std::size_t>(p)]);
  }
  std::vector<double> cost = in.cost, clo = in.col_lo, chi = in.col_hi, rlo = in.row_lo, rhi = in.row_hi;
  double offset = in.offset;
  std::vector<char> row_gone(m, 0), col_gone(n, 0);
  const auto find = [&](std::size_t i, std::int32_t j) -> Entry * {
    for (auto &e : rows[i])
      if (e.col == j)
        return &e;
    return nullptr;
  };
  const auto drop_from_col = [&](std::int32_t j, std::int32_t i) {
    auto &c = cols[static_cast<std::size_t>(j)];
    c.erase(std::remove(c.begin(), c.end(), i), c.end());
  };
  const auto eligible = [&](std::size_t i) {
    return !row_gone[i] && rows[i].size() >= 2 && rows[i].size() <= static_cast<std::size_t>(opt.max_row_length) && !std::isinf(rlo[i]) && rlo[i] == rhi[i];
  };
  std::vector<std::int32_t> queue;
  std::vector<char> queued(m, 0);
  const auto push = [&](std::size_t i) {
    if (!queued[i] && eligible(i)) {
      queued[i] = 1;
      queue.push_back(static_cast<std::int32_t>(i));
    }
  };
  // shortest rows first: doubletons, then small implied-free rows
  {
    std::vector<std::int32_t> order;
    for (std::size_t i = 0; i < m; ++i)
      if (eligible(i))
        order.push_back(static_cast<std::int32_t>(i));
    std::stable_sort(order.begin(), order.end(), [&](std::int32_t a, std::int32_t b) { return rows[static_cast<std::size_t>(a)].size() < rows[static_cast<std::size_t>(b)].size(); });
    for (const auto i : order) {
      queued[static_cast<std::size_t>(i)] = 1;
      queue.push_back(i);
    }
  }
  // Bounds on x_j implied by the CURRENT rows (any row containing x_j) and the explicit bounds of the other columns; x_j is implied free
  // when they lie inside its own bounds, so dropping those bounds on elimination loses nothing. Computed on demand from the current
  // system, so a later decision can never rely on a bound that an earlier elimination already dropped.
  const auto implied_free = [&](std::int32_t j) {
    const auto sj = static_cast<std::size_t>(j);
    double ilo = -kInf, ihi = kInf;
    for (const auto ir : cols[sj]) {
      const auto r = static_cast<std::size_t>(ir);
      if (row_gone[r])
        continue;
      double a = 0.0, omin = 0.0, omax = 0.0;
      int omin_inf = 0, omax_inf = 0;
      for (const auto &e : rows[r]) {
        if (e.col == j) {
          a = e.val;
          continue;
        }
        const auto k = static_cast<std::size_t>(e.col);
        if (e.val > 0) {
          if (std::isinf(clo[k])) ++omin_inf; else omin += e.val * clo[k];
          if (std::isinf(chi[k])) ++omax_inf; else omax += e.val * chi[k];
        } else {
          if (std::isinf(chi[k])) ++omin_inf; else omin += e.val * chi[k];
          if (std::isinf(clo[k])) ++omax_inf; else omax += e.val * clo[k];
        }
      }
      if (std::abs(a) < 1e-9)
        continue;
      // rlo <= a x_j + S <= rhi with S in [omin, omax]:  a x_j >= rlo - omax,  a x_j <= rhi - omin
      double lo_ax = -kInf, hi_ax = kInf;
      if (!std::isinf(rlo[r]) && omax_inf == 0) lo_ax = rlo[r] - omax;
      if (!std::isinf(rhi[r]) && omin_inf == 0) hi_ax = rhi[r] - omin;
      double xl = -kInf, xh = kInf;
      if (a > 0) {
        if (!std::isinf(lo_ax)) xl = lo_ax / a;
        if (!std::isinf(hi_ax)) xh = hi_ax / a;
      } else {
        if (!std::isinf(hi_ax)) xl = hi_ax / a;
        if (!std::isinf(lo_ax)) xh = lo_ax / a;
      }
      ilo = std::max(ilo, xl);
      ihi = std::min(ihi, xh);
    }
    const double ftol = 1e-9 * (1.0 + (std::isinf(ilo) ? 0.0 : std::abs(ilo)) + (std::isinf(ihi) ? 0.0 : std::abs(ihi)));
    const bool lo_ok = std::isinf(clo[sj]) || (!std::isinf(ilo) && ilo >= clo[sj] - ftol);
    const bool hi_ok = std::isinf(chi[sj]) || (!std::isinf(ihi) && ihi <= chi[sj] + ftol);
    return lo_ok && hi_ok;
  };
  const double tol = 1e-9;
  std::size_t head = 0;
  std::vector<Entry> others;
  while (head < queue.size()) {
    const auto i = static_cast<std::size_t>(queue[head++]);
    queued[i] = 0;
    if (!eligible(i))
      continue;
    const std::size_t len = rows[i].size();
    const double c = rlo[i];
    // activity bounds of the row (with counts of infinite contributions) for the implied-free test
    double amin = 0.0, amax = 0.0, amax_abs = 0.0;
    int min_inf = 0, max_inf = 0;
    for (const auto &e : rows[i]) {
      const auto k = static_cast<std::size_t>(e.col);
      amax_abs = std::max(amax_abs, std::abs(e.val));
      if (e.val > 0) {
        if (std::isinf(clo[k])) ++min_inf; else amin += e.val * clo[k];
        if (std::isinf(chi[k])) ++max_inf; else amax += e.val * chi[k];
      } else {
        if (std::isinf(chi[k])) ++min_inf; else amin += e.val * chi[k];
        if (std::isinf(clo[k])) ++max_inf; else amax += e.val * clo[k];
      }
    }
    // pick the column to eliminate: allowed pivot, implied free (or doubleton), least fill, then largest pivot
    const Entry *pick = nullptr;
    double pick_fill = kInf;
    for (const auto &e : rows[i]) {
      const auto sj = static_cast<std::size_t>(e.col);
      if (std::abs(e.val) < 1e-9 || std::abs(e.val) * opt.max_coefficient_ratio < amax_abs)
        continue;
      const std::size_t cc = cols[sj].size();
      if (cc - 1 > static_cast<std::size_t>(opt.max_fill))
        continue;
      if (len > 2 && !implied_free(e.col))
        continue;
      const double fill = static_cast<double>(cc - 1) * static_cast<double>(len - 1);
      const double freed = static_cast<double>(len + cc - 1);
      if (len > 2 && fill > freed + opt.extra_fill)
        continue;
      if (!pick || fill < pick_fill || (fill == pick_fill && std::abs(e.val) > std::abs(pick->val))) {
        pick = &e;
        pick_fill = fill;
      }
    }
    if (!pick)
      continue;
    const std::int32_t j = pick->col;
    const double a = pick->val;
    others.clear();
    for (const auto &e : rows[i])
      if (e.col != j)
        others.push_back(e);
    AggregateResult::Record rec{static_cast<std::int32_t>(i), j, a, c, static_cast<std::int64_t>(res.pool_col.size()), static_cast<std::int32_t>(others.size()), 0.0, 0.0, 0.0, 0.0};
    for (const auto &e : others) {
      res.pool_col.push_back(e.col);
      res.pool_val.push_back(e.val);
    }
    // doubleton: the bounds of x_j become bounds of x_k (x_j need not be free)
    if (others.size() == 1) {
      const std::int32_t k = others[0].col;
      const double t = -others[0].val / a; // x_j = c/a + t x_k
      const double lj = clo[static_cast<std::size_t>(j)], uj = chi[static_cast<std::size_t>(j)];
      double lo = -kInf, hi = kInf;
      const double shift = c / a;
      if (t > 0) {
        if (!std::isinf(lj)) lo = (lj - shift) / t;
        if (!std::isinf(uj)) hi = (uj - shift) / t;
      } else {
        if (!std::isinf(uj)) lo = (uj - shift) / t;
        if (!std::isinf(lj)) hi = (lj - shift) / t;
      }
      auto &klo = clo[static_cast<std::size_t>(k)];
      auto &khi = chi[static_cast<std::size_t>(k)];
      rec.other_lo = klo;
      rec.other_hi = khi;
      rec.col_lo = lj;
      rec.col_hi = uj;
      const double nlo = std::max(klo, lo), nhi = std::min(khi, hi);
      if (nlo > nhi + tol * (1.0 + std::abs(nlo) + std::abs(nhi))) {
        res.status = SolveStatus::Infeasible;
        return res;
      }
      klo = nlo;
      khi = std::max(nlo, nhi); // collapse tiny crossings from rounding
    }
    // cost and objective offset
    const double cj = cost[static_cast<std::size_t>(j)];
    if (cj != 0.0) {
      for (const auto &e : others)
        cost[static_cast<std::size_t>(e.col)] -= cj * e.val / a;
      offset += cj * c / a;
    }
    // substitute into the other rows of x_j
    const std::vector<std::int32_t> touched = cols[static_cast<std::size_t>(j)];
    for (const auto ip32 : touched) {
      const auto ip = static_cast<std::size_t>(ip32);
      if (ip == i || row_gone[ip])
        continue;
      Entry *ej = find(ip, j);
      if (!ej)
        continue;
      const double v = ej->val;
      rows[ip].erase(rows[ip].begin() + (ej - rows[ip].data()));
      const double shift = v * c / a;
      if (!std::isinf(rlo[ip])) rlo[ip] -= shift;
      if (!std::isinf(rhi[ip])) rhi[ip] -= shift;
      for (const auto &ok : others) {
        const double add = -v * ok.val / a;
        if (Entry *ek = find(ip, ok.col)) {
          const double nv = ek->val + add;
          if (std::abs(nv) <= 1e-12 * std::max(std::abs(ek->val), std::abs(add))) {
            rows[ip].erase(rows[ip].begin() + (ek - rows[ip].data()));
            drop_from_col(ok.col, ip32);
          } else {
            ek->val = nv;
          }
        } else if (add != 0.0) {
          rows[ip].push_back({ok.col, add});
          cols[static_cast<std::size_t>(ok.col)].push_back(ip32);
        }
      }
      push(ip);
    }
    // remove the row and the column
    for (const auto &e : others)
      drop_from_col(e.col, static_cast<std::int32_t>(i));
    rows[i].clear();
    cols[static_cast<std::size_t>(j)].clear();
    row_gone[i] = 1;
    col_gone[static_cast<std::size_t>(j)] = 1;
    res.stack.push_back(rec);
  }
  if (res.stack.empty()) {
    identity();
    return res;
  }
  // ---- build the reduced model
  std::vector<std::int32_t> new_col(n, -1), new_row(m, -1);
  for (std::size_t j = 0; j < n; ++j)
    if (!col_gone[j]) {
      new_col[j] = static_cast<std::int32_t>(res.col_map.size());
      res.col_map.push_back(static_cast<std::int32_t>(j));
    }
  for (std::size_t i = 0; i < m; ++i)
    if (!row_gone[i]) {
      new_row[i] = static_cast<std::int32_t>(res.row_map.size());
      res.row_map.push_back(static_cast<std::int32_t>(i));
    }
  Model &red = res.reduced;
  red.rows = static_cast<std::int32_t>(res.row_map.size());
  red.cols = static_cast<std::int32_t>(res.col_map.size());
  red.offset = offset;
  for (const auto j : res.col_map) {
    red.cost.push_back(cost[static_cast<std::size_t>(j)]);
    red.col_lo.push_back(clo[static_cast<std::size_t>(j)]);
    red.col_hi.push_back(chi[static_cast<std::size_t>(j)]);
  }
  std::vector<std::int32_t> tr, tc;
  std::vector<double> tv;
  for (const auto i : res.row_map) {
    red.row_lo.push_back(rlo[static_cast<std::size_t>(i)]);
    red.row_hi.push_back(rhi[static_cast<std::size_t>(i)]);
    for (const auto &e : rows[static_cast<std::size_t>(i)]) {
      tr.push_back(new_row[static_cast<std::size_t>(i)]);
      tc.push_back(new_col[static_cast<std::size_t>(e.col)]);
      tv.push_back(e.val);
    }
  }
  red.a = SparseMatrix::from_triplets(red.rows, red.cols, tr, tc, tv);
  return res;
}

} // namespace pluss::subsp::math::lp
