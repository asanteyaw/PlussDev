#pragma once

// Cutting-plane separators for the MILP solver (all cuts are globally valid: derived with the ROOT bounds only).
//   Gomory mixed-integer cuts from the simplex tableau (Gomory 1960; Balas et al. 1996), with integer-row detection so row logicals
//   of all-integer rows are treated as integer variables;
//   complemented mixed-integer rounding (c-MIR) on single rows (Marchand & Wolsey 2001) with delta search over row coefficients;
//   lifted knapsack cover cuts on rows over binaries (Balas 1975; Wolsey 1975).
// Numerical safeguards follow common practice: dynamism limit, removal of tiny coefficients by bound relaxation, scaling, minimum
// efficacy, and a parallelism filter when selecting cuts.

#include "pluss/subsp/math/lp/simplex.h"
#include "pluss/subsp/math/lp/sparse.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>

namespace pluss::subsp::math::lp {

struct Cut final {
  std::vector<std::int32_t> index;
  std::vector<double> value;
  double lo{-kInf}, hi{kInf};
  double efficacy{0.0};
  int kind{0}; // 0 Gomory, 1 MIR, 2 cover
};

struct CutContext final {
  const Model &model;
  const SparseMatrix &rows_csr;               // model.a transposed (row access)
  const std::vector<char> &is_int;
  const std::vector<double> &lo, &hi;         // global column bounds
  const std::vector<double> &x;               // LP values of the structurals
  const std::vector<char> &row_is_int;        // row activity is integer (all-integer row with integral coefficients)
  const std::vector<std::int32_t> *vub_var = nullptr; // per column: binary k with  x_j <= vub_coef[j] * y_k  (or -1)
  const std::vector<double> *vub_coef = nullptr;
  const std::vector<double> *debug_x = nullptr;
};

namespace cutdetail {

inline double frac(double v) { return v - std::floor(v); }

// finalise a cut  sum value_j x_j >= rhs  (or <=): clean, scale, compute efficacy at x. Returns false if unusable.
inline bool finalize(std::vector<std::pair<std::int32_t, double>> &terms, double &rhs, bool ge, const CutContext &ctx, Cut &out) {
  // drop zeros, combine
  std::sort(terms.begin(), terms.end());
  std::vector<std::pair<std::int32_t, double>> t;
  for (const auto &e : terms) {
    if (!t.empty() && t.back().first == e.first)
      t.back().second += e.second;
    else
      t.push_back(e);
  }
  double amax = 0.0;
  for (const auto &e : t)
    amax = std::max(amax, std::abs(e.second));
  if (amax < 1e-9 || t.empty())
    return false;
  // remove tiny coefficients by relaxing against bounds (keeps validity)
  std::vector<std::pair<std::int32_t, double>> kept;
  for (const auto &e : t) {
    if (std::abs(e.second) < 1e-9 * amax) {
      const auto j = static_cast<std::size_t>(e.first);
      // sum a x >= rhs: dropping a_j x_j loses at most a_j * (bound that minimises a_j x_j) -> adjust rhs
      const double worst = (ge ? (e.second > 0 ? ctx.lo[j] : ctx.hi[j]) : (e.second > 0 ? ctx.hi[j] : ctx.lo[j]));
      if (std::isinf(worst))
        return false;
      rhs -= e.second * worst;
    } else {
      kept.push_back(e);
    }
  }
  if (kept.empty())
    return false;
  double mn = kInf;
  amax = 0.0;
  for (const auto &e : kept) {
    mn = std::min(mn, std::abs(e.second));
    amax = std::max(amax, std::abs(e.second));
  }
  if (amax / mn > 1e6)
    return false; // too dynamic
  const double scale = 1.0 / amax;
  double act = 0.0, norm2 = 0.0;
  out.index.clear();
  out.value.clear();
  for (const auto &e : kept) {
    out.index.push_back(e.first);
    out.value.push_back(e.second * scale);
    act += e.second * scale * ctx.x[static_cast<std::size_t>(e.first)];
    norm2 += e.second * scale * e.second * scale;
  }
  rhs *= scale;
  const double viol = ge ? rhs - act : act - rhs;
  out.efficacy = viol / std::sqrt(norm2);
  if (ge) {
    out.lo = rhs;
    out.hi = kInf;
  } else {
    out.lo = -kInf;
    out.hi = rhs;
  }
  return out.efficacy > 1e-5 && std::isfinite(rhs);
}

} // namespace cutdetail

// ---------------------------------------------------------------------------------------------------------------- Gomory
// For each basic integer variable with fractional value: tableau row  x_B + sum_j r_j x_j = 0  with nonbasic x_j at a bound.
inline void gomory_cuts(const CutContext &ctx, Simplex &lp, std::vector<Cut> &out, int max_cuts) {
  const std::int32_t n = ctx.model.cols, m = ctx.model.rows;
  std::vector<std::pair<double, std::int32_t>> order; // (fractionality score, position)
  for (std::int32_t p = 0; p < m; ++p) {
    const auto v = lp.basic_variable(p);
    if (v >= n || !ctx.is_int[static_cast<std::size_t>(v)])
      continue;
    const double f = cutdetail::frac(ctx.x[static_cast<std::size_t>(v)]);
    if (f < 0.01 || f > 0.99)
      continue;
    order.push_back({-std::min(f, 1.0 - f), p});
  }
  std::sort(order.begin(), order.end());
  std::vector<double> row;
  for (const auto &po : order) {
    if (static_cast<int>(out.size()) >= max_cuts)
      break;
    const std::int32_t p = po.second;
    const auto bv = lp.basic_variable(p);
    lp.tableau_row(p, row);
    const double beta = ctx.x[static_cast<std::size_t>(bv)];
    const double f0 = cutdetail::frac(beta);
    if (f0 < 0.01 || f0 > 0.99)
      continue;
    // y_j = distance of nonbasic j from its bound: x_B + sum a~_j y_j = beta
    std::vector<std::pair<std::int32_t, double>> terms; // coefficients on x
    double rhs = 1.0;                                   // sum c_j y_j >= 1  ->  sum pi x >= rhs
    bool ok = true;
    for (std::int32_t j = 0; j < n + m && ok; ++j) {
      const double r = row[static_cast<std::size_t>(j)];
      if (r == 0.0)
        continue;
      const VarStatus st = lp.status(j);
      if (st == VarStatus::Basic)
        continue;
      const bool logical = j >= n;
      double lo_j, hi_j;
      bool int_j;
      if (!logical) {
        lo_j = ctx.lo[static_cast<std::size_t>(j)];
        hi_j = ctx.hi[static_cast<std::size_t>(j)];
        int_j = ctx.is_int[static_cast<std::size_t>(j)] != 0;
      } else {
        lo_j = ctx.model.row_lo[static_cast<std::size_t>(j - n)];
        hi_j = ctx.model.row_hi[static_cast<std::size_t>(j - n)];
        int_j = ctx.row_is_int[static_cast<std::size_t>(j - n)] != 0;
      }
      if (st == VarStatus::Free) {
        ok = false;
        break;
      }
      const bool at_lower = st == VarStatus::AtLower;
      const double bound = at_lower ? lo_j : hi_j;
      if (std::isinf(bound)) {
        ok = false;
        break;
      }
      // a~ in terms of y >= 0
      const double a = at_lower ? r : -r;
      double coef; // coefficient of y_j in the cut  sum coef y >= 1
      const bool y_int = int_j && std::abs(bound - std::round(bound)) < 1e-9;
      if (y_int) {
        const double fj = cutdetail::frac(a);
        coef = fj <= f0 ? fj / f0 : (1.0 - fj) / (1.0 - f0);
      } else {
        coef = a >= 0 ? a / f0 : -a / (1.0 - f0);
      }
      if (coef == 0.0)
        continue;
      // substitute y_j back: y = x - lo (lower) / hi - x (upper); logical: x_j = sum_i a_ki x_i
      const double sign = at_lower ? 1.0 : -1.0; // y = sign * (x_j - bound)
      if (!logical) {
        terms.push_back({j, coef * sign});
        rhs += coef * sign * bound;
      } else {
        const auto i = static_cast<std::size_t>(j - n);
        for (std::int64_t q = ctx.rows_csr.start[i]; q < ctx.rows_csr.start[i + 1]; ++q)
          terms.push_back({ctx.rows_csr.index[static_cast<std::size_t>(q)], coef * sign * ctx.rows_csr.value[static_cast<std::size_t>(q)]});
        rhs += coef * sign * bound;
      }
    }
    if (!ok)
      continue;
    Cut cut;
    if (cutdetail::finalize(terms, rhs, true, ctx, cut)) {
      cut.kind = 0;
      out.push_back(std::move(cut));
    }
  }
}

// ------------------------------------------------------------------------------------------------------------------ c-MIR
// Complemented MIR on an (optionally aggregated) row  sum a_j x_j <= beta  (Marchand & Wolsey 2001):
//   * integer variables are complemented to their closer bound (x' >= 0),
//   * continuous variables use the closer of their simple bound and their variable upper bound  x_j <= u y_k  (flow-cover-like cuts),
//   * delta is searched among the integer coefficients (and fractions of them), the cut with the best efficacy is kept.
namespace cutdetail {

struct AggRow {
  std::vector<std::pair<std::int32_t, double>> a; // sparse, sorted by column
  double beta{0.0};
};

inline bool cmir_from_row(const CutContext &ctx, const AggRow &row, Cut &best_out) {
  struct IntTerm { std::int32_t j; double a; bool comp; };
  struct ContTerm { std::int32_t j; double a; int kind; std::int32_t k; double u; }; // kind 0: x-lo, 1: hi-x, 2: u*y_k - x
  std::vector<double> yextra; // extra coefficients on binaries from VUB substitution (parallel to a map below)
  std::vector<std::pair<std::int32_t, double>> ycoef;
  std::vector<ContTerm> conts;
  double beta = row.beta;
  std::vector<std::pair<std::int32_t, double>> base; // integer var coefficients before complementation
  for (const auto &e : row.a) {
    const auto j = e.first;
    const auto sj = static_cast<std::size_t>(j);
    const double a = e.second;
    if (ctx.is_int[sj]) {
      base.push_back({j, a});
      continue;
    }
    const double lo = ctx.lo[sj], hi = ctx.hi[sj], xv = ctx.x[sj];
    const bool lo_f = !std::isinf(lo), hi_f = !std::isinf(hi);
    double d_simple = kInf;
    int simple_kind = -1;
    if (lo_f && (!hi_f || xv - lo <= hi - xv)) {
      d_simple = xv - lo;
      simple_kind = 0;
    } else if (hi_f) {
      d_simple = hi - xv;
      simple_kind = 1;
    }
    double d_vub = kInf;
    std::int32_t k = -1;
    double u = 0.0;
    if (ctx.vub_var != nullptr && (*ctx.vub_var)[sj] >= 0) {
      k = (*ctx.vub_var)[sj];
      u = (*ctx.vub_coef)[sj];
      d_vub = u * ctx.x[static_cast<std::size_t>(k)] - xv;
    }
    if (simple_kind < 0 && k < 0)
      return false;
    if (k >= 0 && d_vub < d_simple - 1e-9) { // substitute x = u y_k - s,  s >= 0
      conts.push_back({j, -a, 2, k, u});
      base.push_back({k, a * u});
    } else if (simple_kind == 0) {
      beta -= a * lo;
      conts.push_back({j, a, 0, -1, 0.0});
    } else {
      beta -= a * hi;
      conts.push_back({j, -a, 1, -1, 0.0});
    }
  }
  // merge integer coefficients
  std::sort(base.begin(), base.end());
  std::vector<IntTerm> ints;
  for (std::size_t q = 0; q < base.size();) {
    std::size_t r = q;
    double a = 0.0;
    while (r < base.size() && base[r].first == base[q].first)
      a += base[r++].second;
    q = r;
    const auto j = base[q - 1].first;
    const auto sj = static_cast<std::size_t>(j);
    if (std::abs(a) < 1e-12)
      continue;
    const double lo = ctx.lo[sj], hi = ctx.hi[sj], xv = ctx.x[sj];
    if (std::isinf(lo) || std::isinf(hi))
      return false;
    const bool comp = (hi - xv) < (xv - lo);
    if (!comp) {
      beta -= a * lo;
      ints.push_back({j, a, false});
    } else {
      beta -= a * hi;
      ints.push_back({j, -a, true});
    }
  }
  if (ints.empty())
    return false;
  std::vector<double> deltas;
  for (const auto &t : ints) {
    const auto sj = static_cast<std::size_t>(t.j);
    const double xs = t.comp ? ctx.hi[sj] - ctx.x[sj] : ctx.x[sj] - ctx.lo[sj];
    if (xs > 1e-6 && std::abs(t.a) > 1e-6)
      deltas.push_back(std::abs(t.a));
  }
  std::sort(deltas.begin(), deltas.end());
  deltas.erase(std::unique(deltas.begin(), deltas.end(), [](double a, double b) { return std::abs(a - b) < 1e-9 * (1 + std::abs(a)); }), deltas.end());
  if (deltas.size() > 8)
    deltas.resize(8);
  const auto basev = deltas;
  for (const auto d : basev)
    for (const double div : {2.0, 4.0, 8.0})
      deltas.push_back(d / div);
  double best_eff = 1e-5;
  bool have = false;
  std::vector<std::pair<std::int32_t, double>> terms;
  for (const double delta : deltas) {
    const double bb = beta / delta;
    const double f0 = frac(bb);
    if (f0 < 0.05 || f0 > 0.95)
      continue;
    terms.clear();
    double rhs = std::floor(bb);
    for (const auto &t : ints) {
      const double ab = t.a / delta;
      const double fj = frac(ab);
      const double g = std::floor(ab) + std::max(0.0, fj - f0) / (1.0 - f0);
      const auto sj = static_cast<std::size_t>(t.j);
      if (!t.comp) {
        terms.push_back({t.j, g});
        rhs += g * ctx.lo[sj];
      } else {
        terms.push_back({t.j, -g});
        rhs -= g * ctx.hi[sj];
      }
    }
    const double cf = 1.0 / (delta * (1.0 - f0));
    for (const auto &c : conts) {
      if (c.a >= 0)
        continue;
      const double h = cf * c.a; // coefficient of the slack s (negative)
      const auto sj = static_cast<std::size_t>(c.j);
      if (c.kind == 0) {
        terms.push_back({c.j, h});
        rhs += h * ctx.lo[sj];
      } else if (c.kind == 1) {
        terms.push_back({c.j, -h});
        rhs -= h * ctx.hi[sj];
      } else { // s = u y_k - x_j  (c.a already carries the sign of the substitution: slack coefficient is -a)
        terms.push_back({c.j, -h});
        terms.push_back({c.k, h * c.u});
      }
    }
    Cut c;
    auto tt = terms;
    double r2 = rhs;
    if (finalize(tt, r2, false, ctx, c) && c.efficacy > best_eff) {
      best_eff = c.efficacy;
      best_out = std::move(c);
      have = true;
    }
  }
  return have;
}

} // namespace cutdetail

inline void mir_cuts(const CutContext &ctx, std::vector<Cut> &out, int max_cuts) {
  const std::int32_t m = ctx.model.rows;
  // row index of every column for aggregation: equality rows containing a column
  std::vector<std::vector<std::int32_t>> eq_rows_of(static_cast<std::size_t>(ctx.model.cols));
  for (std::int32_t i = 0; i < m; ++i) {
    const auto si = static_cast<std::size_t>(i);
    if (ctx.model.row_lo[si] == ctx.model.row_hi[si] && ctx.rows_csr.start[si + 1] - ctx.rows_csr.start[si] <= 60)
      for (auto q = ctx.rows_csr.start[si]; q < ctx.rows_csr.start[si + 1]; ++q)
        eq_rows_of[static_cast<std::size_t>(ctx.rows_csr.index[static_cast<std::size_t>(q)])].push_back(i);
  }
  auto row_of = [&](std::int32_t i, double sg, cutdetail::AggRow &agg, double bound) {
    const auto si = static_cast<std::size_t>(i);
    agg.a.clear();
    for (auto q = ctx.rows_csr.start[si]; q < ctx.rows_csr.start[si + 1]; ++q)
      agg.a.push_back({ctx.rows_csr.index[static_cast<std::size_t>(q)], sg * ctx.rows_csr.value[static_cast<std::size_t>(q)]});
    agg.beta = sg * bound;
  };
  for (std::int32_t i = 0; i < m && static_cast<int>(out.size()) < max_cuts; ++i) {
    const auto si = static_cast<std::size_t>(i);
    if (ctx.rows_csr.start[si + 1] - ctx.rows_csr.start[si] > 200)
      continue;
    for (int side = 0; side < 2; ++side) {
      const double bound = side == 0 ? ctx.model.row_hi[si] : ctx.model.row_lo[si];
      if (std::isinf(bound))
        continue;
      const double sg = side == 0 ? 1.0 : -1.0;
      cutdetail::AggRow agg;
      row_of(i, sg, agg, bound);
      std::vector<char> used(static_cast<std::size_t>(m), 0);
      used[si] = 1;
      Cut best;
      bool have = false;
      for (int depth = 0; depth < (getenv("MIR_NOAGG") ? 1 : 5); ++depth) {
        Cut c;
        if (cutdetail::cmir_from_row(ctx, agg, c) && ctx.debug_x != nullptr) {
          double act = 0.0;
          for (std::size_t q = 0; q < c.index.size(); ++q)
            act += c.value[q] * (*ctx.debug_x)[static_cast<std::size_t>(c.index[q])];
          if (act > c.hi + 1e-6 * (1 + std::abs(act))) {
            double ra = 0.0;
            for (const auto &e : agg.a)
              ra += e.second * (*ctx.debug_x)[static_cast<std::size_t>(e.first)];
            static int printed = 0;
            if (printed++ == 0) {
              std::fprintf(stderr, "  row terms (j a lo hi xlp xdbg int):\n");
              for (const auto &e : agg.a) {
                const auto sj = static_cast<std::size_t>(e.first);
                std::fprintf(stderr, "   %d %g [%g,%g] lp=%g dbg=%g int=%d\n", e.first, e.second, ctx.lo[sj], ctx.hi[sj], ctx.x[sj], (*ctx.debug_x)[sj], static_cast<int>(ctx.is_int[sj]));
              }
              std::fprintf(stderr, "  cut:");
              for (std::size_t q = 0; q < c.index.size(); ++q)
                std::fprintf(stderr, " %g*x%d", c.value[q], c.index[q]);
              std::fprintf(stderr, " <= %g\n", c.hi);
            }
            std::fprintf(stderr, "MIR debug: row %d side %d depth %d: agg row activity at solution %.6g vs beta %.6g (agg has %zu terms); cut act %.6g > hi %.6g\n", i, side, depth, ra, agg.beta, agg.a.size(), act, c.hi);
          }
        }
        if (cutdetail::cmir_from_row(ctx, agg, c) && (!have || c.efficacy > best.efficacy)) {
          best = std::move(c);
          have = true;
        }
        // eliminate a continuous variable that sits strictly between its bounds by adding an equality row
        std::int32_t pick = -1;
        double pick_dist = 1e-6;
        for (const auto &e : agg.a) {
          const auto sj = static_cast<std::size_t>(e.first);
          if (ctx.is_int[sj] || eq_rows_of[sj].empty())
            continue;
          const double d = std::min(std::isinf(ctx.lo[sj]) ? kInf : ctx.x[sj] - ctx.lo[sj], std::isinf(ctx.hi[sj]) ? kInf : ctx.hi[sj] - ctx.x[sj]);
          if (d > pick_dist && !std::isinf(d)) {
            pick_dist = d;
            pick = e.first;
          } else if (std::isinf(d) && pick_dist < 1e6) {
            pick_dist = 1e6;
            pick = e.first;
          }
        }
        if (pick < 0)
          break;
        std::int32_t row = -1;
        for (const auto r : eq_rows_of[static_cast<std::size_t>(pick)])
          if (!used[static_cast<std::size_t>(r)]) {
            row = r;
            break;
          }
        if (row < 0)
          break;
        used[static_cast<std::size_t>(row)] = 1;
        double coef_in_row = 0.0, coef_in_agg = 0.0;
        for (auto q = ctx.rows_csr.start[static_cast<std::size_t>(row)]; q < ctx.rows_csr.start[static_cast<std::size_t>(row) + 1]; ++q)
          if (ctx.rows_csr.index[static_cast<std::size_t>(q)] == pick)
            coef_in_row = ctx.rows_csr.value[static_cast<std::size_t>(q)];
        for (const auto &e : agg.a)
          if (e.first == pick)
            coef_in_agg = e.second;
        if (std::abs(coef_in_row) < 1e-9)
          break;
        const double lambda = -coef_in_agg / coef_in_row;
        for (auto q = ctx.rows_csr.start[static_cast<std::size_t>(row)]; q < ctx.rows_csr.start[static_cast<std::size_t>(row) + 1]; ++q)
          agg.a.push_back({ctx.rows_csr.index[static_cast<std::size_t>(q)], lambda * ctx.rows_csr.value[static_cast<std::size_t>(q)]});
        agg.beta += lambda * ctx.model.row_lo[static_cast<std::size_t>(row)];
        std::sort(agg.a.begin(), agg.a.end());
        std::vector<std::pair<std::int32_t, double>> merged;
        for (const auto &e : agg.a) {
          if (!merged.empty() && merged.back().first == e.first)
            merged.back().second += e.second;
          else
            merged.push_back(e);
        }
        agg.a.clear();
        for (const auto &e : merged)
          if (std::abs(e.second) > 1e-11)
            agg.a.push_back(e);
        if (agg.a.size() > 300)
          break;
      }
      if (have) {
        best.kind = 1;
        out.push_back(std::move(best));
      }
    }
  }
}

// ----------------------------------------------------------------------------------------------------------- lifted covers
inline void cover_cuts(const CutContext &ctx, std::vector<Cut> &out, int max_cuts) {
  const std::int32_t m = ctx.model.rows;
  for (std::int32_t i = 0; i < m && static_cast<int>(out.size()) < max_cuts; ++i) {
    const auto si = static_cast<std::size_t>(i);
    const auto beg = ctx.rows_csr.start[si], end = ctx.rows_csr.start[si + 1];
    if (end - beg < 3 || end - beg > 300)
      continue;
    for (int side = 0; side < 2; ++side) {
      const double bound = side == 0 ? ctx.model.row_hi[si] : ctx.model.row_lo[si];
      if (std::isinf(bound))
        continue;
      const double sg = side == 0 ? 1.0 : -1.0;
      double beta = sg * bound;
      struct Item { std::int32_t j; double a; bool comp; double xs; };
      std::vector<Item> items;
      bool ok = true;
      for (auto q = beg; q < end && ok; ++q) {
        const auto j = ctx.rows_csr.index[static_cast<std::size_t>(q)];
        const auto sj = static_cast<std::size_t>(j);
        const double a = sg * ctx.rows_csr.value[static_cast<std::size_t>(q)];
        if (!ctx.is_int[sj] || ctx.lo[sj] != 0.0 || ctx.hi[sj] != 1.0) { ok = false; break; }
        if (a >= 0) items.push_back({j, a, false, ctx.x[sj]});
        else { beta -= a; items.push_back({j, -a, true, 1.0 - ctx.x[sj]}); } // x = 1 - x'
      }
      if (!ok || beta <= 1e-9)
        continue;
      double total = 0.0;
      for (const auto &t : items)
        total += t.a;
      if (total <= beta + 1e-9)
        continue; // no cover exists
      std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return (1.0 - a.xs) / a.a < (1.0 - b.xs) / b.a; });
      std::vector<Item> cover;
      double w = 0.0;
      for (const auto &t : items) {
        cover.push_back(t);
        w += t.a;
        if (w > beta + 1e-9)
          break;
      }
      // minimalise: drop items (largest slack first) while still a cover
      std::sort(cover.begin(), cover.end(), [](const Item &a, const Item &b) { return a.xs < b.xs; });
      for (std::size_t k = 0; k < cover.size();) {
        if (w - cover[k].a > beta + 1e-9) {
          w -= cover[k].a;
          cover.erase(cover.begin() + static_cast<std::ptrdiff_t>(k));
        } else {
          ++k;
        }
      }
      if (cover.size() < 2)
        continue;
      double amax = 0.0;
      for (const auto &t : cover)
        amax = std::max(amax, t.a);
      // extended cover: add items at least as heavy as the heaviest cover item
      std::vector<Item> ext = cover;
      for (const auto &t : items) {
        bool in = false;
        for (const auto &c : cover)
          in |= c.j == t.j;
        if (!in && t.a >= amax - 1e-12)
          ext.push_back(t);
      }
      // cut: sum_{ext} x' <= |cover| - 1, back-substitute complemented variables x' = 1 - x
      std::vector<std::pair<std::int32_t, double>> terms;
      double rhs = static_cast<double>(cover.size()) - 1.0;
      for (const auto &t : ext) {
        if (!t.comp)
          terms.push_back({t.j, 1.0});
        else {
          terms.push_back({t.j, -1.0});
          rhs -= 1.0;
        }
      }
      Cut c;
      if (cutdetail::finalize(terms, rhs, false, ctx, c)) {
        c.kind = 2;
        out.push_back(std::move(c));
      }
    }
  }
}


// ---------------------------------------------------------------------------------------------------------------- clique
// Conflict-graph clique cuts (Johnson & Padberg 1982; Atamturk, Nemhauser, Savelsbergh 2000). A row  sum w_l lit_l <= beta  over binary
// literals (x_j or 1 - x_j) makes the p heaviest literals pairwise conflicting when w_(p-1) + w_p > beta, i.e. a clique
// sum_{l in K} lit_l <= 1. Cliques of all rows define the conflict graph; fractional points are cut by greedily extending cliques
// (seeded from the literals with the largest LP value, extended over all literals adjacent to the whole set) to maximal ones.
inline void clique_cuts(const CutContext &ctx, std::vector<Cut> &out, int max_cuts) {
  const std::int32_t m = ctx.model.rows;
  const auto lit_count = static_cast<std::size_t>(2 * ctx.model.cols);
  std::vector<std::vector<std::int32_t>> cliques;
  std::vector<std::vector<std::int32_t>> member(lit_count); // clique ids per literal
  for (std::int32_t i = 0; i < m; ++i) {
    const auto si = static_cast<std::size_t>(i);
    const auto beg = ctx.rows_csr.start[si], end = ctx.rows_csr.start[si + 1];
    if (end - beg < 2 || end - beg > 2000)
      continue;
    for (int side = 0; side < 2; ++side) {
      const double bound = side == 0 ? ctx.model.row_hi[si] : ctx.model.row_lo[si];
      if (std::isinf(bound))
        continue;
      const double sg = side == 0 ? 1.0 : -1.0;
      double beta = sg * bound;
      std::vector<std::pair<double, std::int32_t>> items; // (weight, literal)
      bool ok = true;
      for (auto q = beg; q < end && ok; ++q) {
        const auto j = ctx.rows_csr.index[static_cast<std::size_t>(q)];
        const auto sj = static_cast<std::size_t>(j);
        if (!ctx.is_int[sj] || ctx.lo[sj] != 0.0 || ctx.hi[sj] != 1.0) {
          ok = false;
          break;
        }
        const double a = sg * ctx.rows_csr.value[static_cast<std::size_t>(q)];
        if (a >= 0)
          items.push_back({a, 2 * j});
        else {
          beta -= a;
          items.push_back({-a, 2 * j + 1});
        }
      }
      if (!ok || items.size() < 2)
        continue;
      std::sort(items.begin(), items.end(), [](const auto &x, const auto &y) { return x.first > y.first; });
      std::size_t p = 0;
      for (std::size_t k = 1; k < items.size(); ++k)
        if (items[k - 1].first + items[k].first > beta + 1e-9)
          p = k + 1;
        else
          break;
      if (p < 2)
        continue;
      std::vector<std::int32_t> k;
      for (std::size_t t = 0; t < p; ++t)
        k.push_back(items[t].second);
      const auto id = static_cast<std::int32_t>(cliques.size());
      for (const auto l : k)
        member[static_cast<std::size_t>(l)].push_back(id);
      cliques.push_back(std::move(k));
    }
  }
  if (cliques.empty())
    return;
  const auto value = [&](std::int32_t l) {
    const double v = ctx.x[static_cast<std::size_t>(l >> 1)];
    return (l & 1) ? 1.0 - v : v;
  };
  const auto adjacent = [&](std::int32_t a, std::int32_t b) {
    if (a == b)
      return false;
    if ((a >> 1) == (b >> 1)) // x and 1 - x cannot both be 1
      return true;
    const auto &ma = member[static_cast<std::size_t>(a)], &mb = member[static_cast<std::size_t>(b)];
    std::size_t u = 0, v = 0;
    while (u < ma.size() && v < mb.size()) {
      if (ma[u] == mb[v])
        return true;
      ma[u] < mb[v] ? ++u : ++v;
    }
    return false;
  };
  std::vector<std::int32_t> seeds;
  for (std::size_t l = 0; l < lit_count; ++l)
    if (!member[l].empty() && value(static_cast<std::int32_t>(l)) > 1e-6)
      seeds.push_back(static_cast<std::int32_t>(l));
  std::sort(seeds.begin(), seeds.end(), [&](auto a, auto b) { return value(a) > value(b); });
  if (seeds.size() > 800)
    seeds.resize(800);
  std::vector<std::vector<std::int32_t>> seen;
  for (const auto seed : seeds) {
    if (static_cast<int>(out.size()) >= max_cuts)
      break;
    std::vector<std::int32_t> set{seed}, cand;
    for (const auto id : member[static_cast<std::size_t>(seed)])
      for (const auto l : cliques[static_cast<std::size_t>(id)])
        if (l != seed)
          cand.push_back(l);
    std::sort(cand.begin(), cand.end());
    cand.erase(std::unique(cand.begin(), cand.end()), cand.end());
    std::sort(cand.begin(), cand.end(), [&](auto a, auto b) { return value(a) > value(b); });
    for (const auto l : cand) {
      bool all = true;
      for (const auto t : set)
        if (!adjacent(l, t)) {
          all = false;
          break;
        }
      if (all)
        set.push_back(l);
    }
    double total = 0.0;
    for (const auto l : set)
      total += value(l);
    if (set.size() < 2 || total < 1.0 + 1e-3)
      continue;
    std::sort(set.begin(), set.end());
    bool dup = false;
    for (const auto &e : seen)
      dup |= e == set;
    if (dup)
      continue;
    seen.push_back(set);
    std::vector<std::pair<std::int32_t, double>> terms;
    double rhs = 1.0;
    for (const auto l : set) {
      if (l & 1) {
        terms.push_back({l >> 1, -1.0});
        rhs -= 1.0;
      } else {
        terms.push_back({l >> 1, 1.0});
      }
    }
    Cut c;
    if (cutdetail::finalize(terms, rhs, false, ctx, c)) {
      c.kind = 3;
      out.push_back(std::move(c));
    }
  }
}

// selection: highest efficacy first, rejecting near-parallel cuts
inline std::vector<Cut> select_cuts(std::vector<Cut> pool, std::int32_t cols, std::size_t max_keep, double max_parallelism = 0.95) {
  std::sort(pool.begin(), pool.end(), [](const Cut &a, const Cut &b) { return a.efficacy > b.efficacy; });
  std::vector<Cut> kept;
  std::vector<double> dense(static_cast<std::size_t>(cols), 0.0);
  auto norm = [](const Cut &c) {
    double s = 0.0;
    for (const double v : c.value)
      s += v * v;
    return std::sqrt(s);
  };
  std::vector<double> norms;
  for (auto &c : pool) {
    if (kept.size() >= max_keep)
      break;
    const double nc = norm(c);
    bool parallel = false;
    std::fill(dense.begin(), dense.end(), 0.0);
    for (std::size_t k = 0; k < c.index.size(); ++k)
      dense[static_cast<std::size_t>(c.index[k])] = c.value[k];
    for (std::size_t q = 0; q < kept.size() && !parallel; ++q) {
      double dot = 0.0;
      for (std::size_t k = 0; k < kept[q].index.size(); ++k)
        dot += kept[q].value[k] * dense[static_cast<std::size_t>(kept[q].index[k])];
      parallel = std::abs(dot) / (nc * norms[q]) > max_parallelism;
    }
    if (!parallel) {
      norms.push_back(nc);
      kept.push_back(std::move(c));
    }
  }
  return kept;
}

} // namespace pluss::subsp::math::lp
