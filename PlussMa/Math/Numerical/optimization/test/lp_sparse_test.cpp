// Verification of the sparse industry-scale LP / MILP stack (pluss::subsp::math::lp): revised simplex, presolve, interior point with
// crossover, branch-and-cut and the MPS reader.
//
// Oracles: the HiGHS reference cases already used for the dense solvers (linprog_reference.h / milp_reference.h), exhaustive
// enumeration of tiny binary programs, closed-form models, independent primal/dual/Farkas certificates, and regression models for
// every soundness bug found while scaling the stack up (presolve verdicts that disagreed with HiGHS).
#include "linprog_reference.h"
#include "milp_reference.h"
#include "pluss/subsp/math/lp/aggregate.h"
#include "pluss/subsp/math/lp/ipm.h"
#include "pluss/subsp/math/lp/lu.h"
#include "pluss/subsp/math/lp/mip.h"
#include "pluss/subsp/math/lp/mps.h"
#include "pluss/subsp/math/lp/series_bridge.h"
#include "pluss/subsp/math/lp/solve.h"

#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace lp = pluss::subsp::math::lp;

namespace {
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}

lp::Model make_model(int n, int mi, int me, const std::vector<double> &ai, const std::vector<double> &ae, const std::vector<double> &c,
                     const std::vector<double> &bi, const std::vector<double> &be, const std::vector<double> &lb, const std::vector<double> &ub,
                     const std::vector<double> &integer = {}) {
  lp::Model M;
  M.rows = mi + me;
  M.cols = n;
  M.cost = c;
  M.col_lo = lb;
  M.col_hi = ub;
  if (!integer.empty()) {
    M.integer.assign(static_cast<std::size_t>(n), 0);
    for (int j = 0; j < n; ++j)
      M.integer[static_cast<std::size_t>(j)] = integer[static_cast<std::size_t>(j)] != 0.0;
  }
  std::vector<std::int32_t> r, cc;
  std::vector<double> v;
  for (int i = 0; i < mi; ++i)
    for (int j = 0; j < n; ++j)
      if (ai[static_cast<std::size_t>(i * n + j)] != 0.0) {
        r.push_back(i);
        cc.push_back(j);
        v.push_back(ai[static_cast<std::size_t>(i * n + j)]);
      }
  for (int i = 0; i < me; ++i)
    for (int j = 0; j < n; ++j)
      if (ae[static_cast<std::size_t>(i * n + j)] != 0.0) {
        r.push_back(mi + i);
        cc.push_back(j);
        v.push_back(ae[static_cast<std::size_t>(i * n + j)]);
      }
  M.a = lp::SparseMatrix::from_triplets(mi + me, n, r, cc, v);
  for (int i = 0; i < mi; ++i) {
    M.row_lo.push_back(-INFINITY);
    M.row_hi.push_back(bi[static_cast<std::size_t>(i)]);
  }
  for (int i = 0; i < me; ++i) {
    M.row_lo.push_back(be[static_cast<std::size_t>(i)]);
    M.row_hi.push_back(be[static_cast<std::size_t>(i)]);
  }
  return M;
}

double primal_violation(const lp::Model &M, const std::vector<double> &x, bool integral) {
  std::vector<double> act(static_cast<std::size_t>(M.rows), 0.0);
  M.a.multiply_add(1.0, x, act);
  double v = 0.0;
  for (int i = 0; i < M.rows; ++i) {
    v = std::max(v, M.row_lo[static_cast<std::size_t>(i)] - act[static_cast<std::size_t>(i)]);
    v = std::max(v, act[static_cast<std::size_t>(i)] - M.row_hi[static_cast<std::size_t>(i)]);
  }
  for (int j = 0; j < M.cols; ++j) {
    v = std::max(v, M.col_lo[static_cast<std::size_t>(j)] - x[static_cast<std::size_t>(j)]);
    v = std::max(v, x[static_cast<std::size_t>(j)] - M.col_hi[static_cast<std::size_t>(j)]);
    if (integral && !M.integer.empty() && M.integer[static_cast<std::size_t>(j)])
      v = std::max(v, std::abs(x[static_cast<std::size_t>(j)] - std::round(x[static_cast<std::size_t>(j)])));
  }
  return v;
}

double objective_of(const lp::Model &M, const std::vector<double> &x) {
  double o = M.offset;
  for (int j = 0; j < M.cols; ++j)
    o += M.cost[static_cast<std::size_t>(j)] * x[static_cast<std::size_t>(j)];
  return o;
}

// Dual certificate in the original space: d = c - A^T y, and the dual objective assembled from the active bounds must equal the primal one.
double dual_gap(const lp::Model &M, const lp::LpSolution &s, double &dual_feas) {
  std::vector<double> d(M.cost);
  M.a.multiply_transpose_add(-1.0, s.row_dual, d);
  double dobj = M.offset;
  dual_feas = 0.0;
  for (int j = 0; j < M.cols; ++j) {
    const double dj = d[static_cast<std::size_t>(j)];
    dual_feas = std::max(dual_feas, std::abs(dj - s.reduced[static_cast<std::size_t>(j)]));
    if (dj > 1e-7) {
      if (std::isinf(M.col_lo[static_cast<std::size_t>(j)])) dual_feas = std::max(dual_feas, dj); else dobj += dj * M.col_lo[static_cast<std::size_t>(j)];
    } else if (dj < -1e-7) {
      if (std::isinf(M.col_hi[static_cast<std::size_t>(j)])) dual_feas = std::max(dual_feas, -dj); else dobj += dj * M.col_hi[static_cast<std::size_t>(j)];
    }
  }
  for (int i = 0; i < M.rows; ++i) {
    const double yi = s.row_dual[static_cast<std::size_t>(i)];
    if (yi > 1e-7) {
      if (std::isinf(M.row_lo[static_cast<std::size_t>(i)])) dual_feas = std::max(dual_feas, yi); else dobj += yi * M.row_lo[static_cast<std::size_t>(i)];
    } else if (yi < -1e-7) {
      if (std::isinf(M.row_hi[static_cast<std::size_t>(i)])) dual_feas = std::max(dual_feas, -yi); else dobj += yi * M.row_hi[static_cast<std::size_t>(i)];
    }
  }
  return std::abs(dobj - s.objective) / (1.0 + std::abs(s.objective));
}

// Farkas certificate: for y (either sign), the largest row-activity y^T r over [row_lo,row_hi] must lie strictly below the smallest
// y^T A x over the variable box -> no x can satisfy all rows.
bool farkas_valid(const lp::Model &M, const std::vector<double> &y) {
  for (const double sg : {1.0, -1.0}) {
    double rows_max = 0.0, scale = 1.0;
    bool finite = true;
    for (int i = 0; i < M.rows && finite; ++i) {
      const double yi = sg * y[static_cast<std::size_t>(i)];
      if (yi > 0) { if (std::isinf(M.row_hi[static_cast<std::size_t>(i)])) { if (yi > 1e-9) finite = false; } else rows_max += yi * M.row_hi[static_cast<std::size_t>(i)]; }
      else if (yi < 0) { if (std::isinf(M.row_lo[static_cast<std::size_t>(i)])) { if (-yi > 1e-9) finite = false; } else rows_max += yi * M.row_lo[static_cast<std::size_t>(i)]; }
    }
    if (!finite)
      continue;
    std::vector<double> g(static_cast<std::size_t>(M.cols), 0.0), ys(y);
    for (auto &v : ys)
      v *= sg;
    M.a.multiply_transpose_add(1.0, ys, g);
    double box_min = 0.0;
    for (int j = 0; j < M.cols && finite; ++j) {
      const double gj = g[static_cast<std::size_t>(j)];
      if (gj > 0) { if (std::isinf(M.col_lo[static_cast<std::size_t>(j)])) { if (gj > 1e-9) finite = false; } else box_min += gj * M.col_lo[static_cast<std::size_t>(j)]; }
      else if (gj < 0) { if (std::isinf(M.col_hi[static_cast<std::size_t>(j)])) { if (-gj > 1e-9) finite = false; } else box_min += gj * M.col_hi[static_cast<std::size_t>(j)]; }
    }
    if (finite && box_min > rows_max + 1e-9 * (1.0 + std::abs(rows_max) + std::abs(box_min)) * scale)
      return true;
  }
  return false;
}

// Exhaustive enumeration of a pure-binary model: {feasible, best objective}.
std::vector<double> g_best_x;
std::pair<bool, double> brute_binary(const lp::Model &M) {
  bool any = false;
  g_best_x.clear();
  double best = INFINITY;
  const int n = M.cols;
  std::vector<double> x(static_cast<std::size_t>(n));
  for (std::uint32_t bits = 0; bits < (1u << n); ++bits) {
    for (int j = 0; j < n; ++j)
      x[static_cast<std::size_t>(j)] = (bits >> j) & 1u;
    if (primal_violation(M, x, false) > 1e-9)
      continue;
    any = true;
    if (objective_of(M, x) < best) { best = objective_of(M, x); g_best_x = x; }
  }
  return {any, best};
}

struct Lcg {
  std::uint64_t s;
  double uni() { s = s * 6364136223846793005ULL + 1442695040888963407ULL; return static_cast<double>(s >> 11) / 9007199254740992.0; }
  int range(int lo, int hi) { return lo + static_cast<int>(uni() * (hi - lo + 1)); }
};

std::map<std::string, int> tally;
} // namespace

int main() {
  try {
    // ---- 1. HiGHS reference LPs through the sparse simplex (primal + dual certificates) and the sparse interior point (+ crossover)
    for (const auto &g : linprog_reference::groups()) {
      for (std::size_t l = 0; l < g.lanes.size(); ++l) {
        const auto &lane = g.lanes[l];
        const std::string tag = std::string(g.name) + " lane " + std::to_string(l);
        if (lane.status != 0 && lane.status != 1 && lane.status != 2)
          continue;
        const auto M = make_model(g.n, g.mi, g.me, g.ai, g.ae, lane.c, lane.bi, lane.be, lane.lb, lane.ub);
        // simplex through the presolving facade
        const auto s = lp::solve(M);
        const int st = static_cast<int>(s.status);
        expect(st == lane.status, tag + ": simplex status " + std::to_string(st) + " expected " + std::to_string(lane.status));
        if (st == 0) {
          expect(primal_violation(M, s.x, false) < 1e-6, tag + ": simplex primal infeasible");
          expect(std::abs(s.objective - lane.objective) < 1e-6 * (1 + std::abs(lane.objective)), tag + ": simplex objective vs HiGHS");
          expect(std::abs(objective_of(M, s.x) - s.objective) < 1e-6 * (1 + std::abs(s.objective)), tag + ": simplex objective inconsistent");
          double dfeas = 0.0;
          const double gap = dual_gap(M, s, dfeas);
          expect(gap < 1e-6 && dfeas < 1e-6, tag + ": simplex dual certificate (gap " + std::to_string(gap) + ", feas " + std::to_string(dfeas) + ")");
        } else if (st == 1) {
          expect(!s.farkas.empty() && farkas_valid(M, s.farkas), tag + ": missing / invalid Farkas certificate");
        } else if (st == 2) {
          expect(!s.ray.empty(), tag + ": missing unbounded ray");
          std::vector<double> az(static_cast<std::size_t>(M.rows), 0.0);
          M.a.multiply_add(1.0, s.ray, az);
          double cz = 0.0;
          for (int j = 0; j < M.cols; ++j) cz += M.cost[static_cast<std::size_t>(j)] * s.ray[static_cast<std::size_t>(j)];
          expect(cz < -1e-9, tag + ": ray does not improve the objective");
        }
        {
          lp::LpOptions race; // force the concurrent simplex / interior-point race (and the interior point alone) on tiny models
          race.method = lp::LpMethod::Race;
          race.race_min_rows = 1;
          race.race_ipm_delay_seconds = 0.0;
          race.race_min_nonzeros = 1;
          const auto rr = lp::solve(M, race);
          expect(static_cast<int>(rr.status) == lane.status, tag + ": race status " + std::to_string(static_cast<int>(rr.status)));
          if (lane.status == 0) {
            expect(std::abs(rr.objective - lane.objective) < 1e-6 * (1 + std::abs(lane.objective)), tag + ": race objective");
            expect(primal_violation(M, rr.x, false) < 1e-5, tag + ": race primal feasibility");
          }
          lp::LpOptions only_ipm;
          only_ipm.method = lp::LpMethod::InteriorPoint;
          const auto ri = lp::solve(M, only_ipm);
          expect(static_cast<int>(ri.status) == lane.status, tag + ": ipm-only status");
          ++tally["race / ipm-only solves"];
        }
        ++tally[std::string("simplex ") + (st == 0 ? "optimal" : st == 1 ? "infeasible" : "unbounded")];
        // interior point
        const auto ip = lp::solve_ipm(M);
        const int ist = static_cast<int>(ip.status);
        expect(ist == lane.status, tag + ": ipm status " + std::to_string(ist) + " expected " + std::to_string(lane.status));
        if (ist == 0) {
          expect(primal_violation(M, ip.x, false) < 1e-5, tag + ": ipm primal infeasible");
          expect(std::abs(ip.objective - lane.objective) < 1e-5 * (1 + std::abs(lane.objective)), tag + ": ipm objective vs HiGHS");
        }
        ++tally[std::string("ipm ") + (ist == 0 ? "optimal" : ist == 1 ? "infeasible" : "unbounded")];
      }
    }

    // ---- 2. HiGHS reference MILPs through branch-and-cut (default options: presolve, cuts, heuristics)
    {
      int idx = 0;
      for (const auto &cs : milp_reference::cases()) {
        const std::string tag = std::string("milp ") + cs.kind + " #" + std::to_string(idx++);
        const auto M = make_model(cs.n, cs.mi, cs.me, cs.ai, cs.ae, cs.c, cs.bi, cs.be, cs.lb, cs.ub, cs.integer);
        lp::MipOptions mo;
        mo.time_limit_seconds = 60;
        const auto r = lp::solve_mip(M, mo);
        const int st = static_cast<int>(r.status);
        expect(st == cs.status, tag + ": status " + std::to_string(st) + " expected " + std::to_string(cs.status));
        if (st == 0) {
          expect(primal_violation(M, r.x, true) < 1e-6, tag + ": incumbent infeasible / non-integral");
          const double val = objective_of(M, r.x);
          expect(std::abs(val - cs.objective) < 1e-6 * (1 + std::abs(cs.objective)), tag + ": objective " + std::to_string(val) + " vs HiGHS " + std::to_string(cs.objective));
          expect(std::abs(r.objective - val) < 1e-7 * (1 + std::abs(val)), tag + ": reported objective inconsistent");
        }
        ++tally[std::string("milp ") + (st == 0 ? "optimal" : st == 1 ? "infeasible" : "unbounded")];
      }
    }

    // ---- 3. random tiny binary programs vs exhaustive enumeration (with and without presolve / cuts / heuristics)
    {
      Lcg rng{12345};
      int count = 0;
      for (int t = 0; t < 400; ++t) {
        const int n = rng.range(3, 10), mi = rng.range(1, 4), me = rng.range(0, 2);
        std::vector<double> ai(static_cast<std::size_t>(mi * n)), ae(static_cast<std::size_t>(me * n)), c(static_cast<std::size_t>(n)), bi(static_cast<std::size_t>(mi)), be(static_cast<std::size_t>(me));
        for (auto &v : ai) v = rng.uni() < 0.3 ? 0.0 : rng.range(-5, 8);
        for (auto &v : ae) v = rng.uni() < 0.3 ? 0.0 : rng.range(-3, 5);
        for (auto &v : c) v = rng.range(-9, 9);
        std::vector<double> x0(static_cast<std::size_t>(n));
        for (auto &v : x0) v = rng.uni() < 0.5 ? 1.0 : 0.0;
        const bool feasible_by_construction = rng.uni() < 0.7;
        for (int i = 0; i < mi; ++i) { double a = 0; for (int j = 0; j < n; ++j) a += ai[static_cast<std::size_t>(i * n + j)] * x0[static_cast<std::size_t>(j)]; bi[static_cast<std::size_t>(i)] = feasible_by_construction ? a + rng.range(0, 2) : rng.range(-4, 4); }
        for (int i = 0; i < me; ++i) { double a = 0; for (int j = 0; j < n; ++j) a += ae[static_cast<std::size_t>(i * n + j)] * x0[static_cast<std::size_t>(j)]; be[static_cast<std::size_t>(i)] = feasible_by_construction ? a : rng.range(-3, 5); }
        const auto M = make_model(n, mi, me, ai, ae, c, bi, be, std::vector<double>(static_cast<std::size_t>(n), 0.0), std::vector<double>(static_cast<std::size_t>(n), 1.0), std::vector<double>(static_cast<std::size_t>(n), 1.0));
        const auto ref = brute_binary(M);
        for (int vi = 0; vi < 3; ++vi) {
          const int variant = std::getenv("LP_DEBUG_SOLUTION") ? (vi + 1) % 3 : vi;
          lp::MipOptions mo;
          mo.presolve = variant != 1;
          mo.cut_rounds = variant == 2 ? 0 : 10;
          mo.heuristics = variant != 2;
          if (variant == 1 && ref.first && std::getenv("LP_DEBUG_SOLUTION")) mo.debug_solution = &g_best_x;
          const auto r = lp::solve_mip(M, mo);
          const std::string tag = "random binary #" + std::to_string(t) + " variant " + std::to_string(variant);
          expect(static_cast<int>(r.status) == (ref.first ? 0 : 1), tag + ": status " + std::to_string(static_cast<int>(r.status)) + (ref.first ? " (feasible)" : " (infeasible)"));
          if (ref.first) {
            expect(primal_violation(M, r.x, true) < 1e-6, tag + ": incumbent infeasible");
            expect(std::abs(objective_of(M, r.x) - ref.second) < 1e-7, tag + ": objective " + std::to_string(objective_of(M, r.x)) + " vs enumeration " + std::to_string(ref.second));
          }
        }
        ++count;
      }
      tally["random binary programs vs enumeration (x3 option sets)"] = count;
    }


    // ---- 3b. set packing / partitioning / covering (clique cuts and conflict structure) vs enumeration
    {
      Lcg rng{777};
      for (int t = 0; t < 200; ++t) {
        const int n = rng.range(6, 12), mi = rng.range(3, 7);
        std::vector<double> ai, bi(static_cast<std::size_t>(mi)), c(static_cast<std::size_t>(n));
        for (int i = 0; i < mi; ++i) {
          const int kind = rng.range(0, 2); // 0: packing (<= 1), 1: covering (>= 1 written as -x <= -1), 2: <= 2 with weights
          for (int j = 0; j < n; ++j) {
            const bool in = rng.uni() < 0.45;
            ai.push_back(kind == 1 ? (in ? -1.0 : 0.0) : (in ? (kind == 2 ? rng.range(1, 3) : 1.0) : 0.0));
          }
          bi[static_cast<std::size_t>(i)] = kind == 0 ? 1.0 : kind == 1 ? -1.0 : 3.0;
        }
        for (auto &v : c) v = rng.range(-9, 4);
        const auto M = make_model(n, mi, 0, ai, {}, c, bi, {}, std::vector<double>(static_cast<std::size_t>(n), 0.0), std::vector<double>(static_cast<std::size_t>(n), 1.0), std::vector<double>(static_cast<std::size_t>(n), 1.0));
        const auto ref = brute_binary(M);
        const auto r = lp::solve_mip(M, {});
        const std::string tag = "set packing #" + std::to_string(t);
        expect(static_cast<int>(r.status) == (ref.first ? 0 : 1), tag + ": status");
        if (ref.first)
          expect(std::abs(r.objective - ref.second) < 1e-7, tag + ": objective " + std::to_string(r.objective) + " vs " + std::to_string(ref.second));
      }
      tally["set-packing/covering programs vs enumeration"] = 200;
    }

    // ---- 4. presolve soundness regressions
    {
      // (a) infeasible only through combined rows: a.x <= 1 and a.x >= 3. Singleton-row tightening produced crossing integer bounds
      //     which dual fixing then silently overwrote, so the model was reported feasible.
      const std::vector<double> a{0.16522505402339635, -0.8902205348286676, 0.07881512017237603, 0.2140679175605372, 0.5894049232058469};
      std::vector<double> ai(a), neg;
      for (double v : a) neg.push_back(-v);
      ai.insert(ai.end(), neg.begin(), neg.end());
      const auto M = make_model(5, 2, 0, ai, {}, {0.0197, -0.281, 0.530, 0.559, -0.469}, {1.0, -3.0}, {}, std::vector<double>(5, 0.0), std::vector<double>(5, 3.0), std::vector<double>(5, 1.0));
      const auto r = lp::solve_mip(M, {});
      expect(r.status == lp::SolveStatus::Infeasible, "presolve regression (a): infeasible model reported as solved");
      // (b) coefficient tightening on an integer column with a nonzero lower bound and a negative coefficient:
      //     max x0 + x1 + x2   s.t.  3 x0 - 2 x1 + x2 <= 4 + 3 ... enumerated exhaustively over the box [1,2]^3
      Lcg rng{99};
      for (int t = 0; t < 300; ++t) {
        const int n = 3;
        std::vector<double> row(3), c(3);
        for (auto &v : row) v = rng.range(-4, 4);
        for (auto &v : c) v = rng.range(-5, 5);
        const double rhs = rng.range(-2, 8);
        const auto Mi = make_model(n, 1, 0, row, {}, c, {rhs}, {}, {1, 1, 1}, {2, 2, 2}, {1, 1, 1});
        bool any = false;
        double best = INFINITY;
        for (int x0 = 1; x0 <= 2; ++x0) for (int x1 = 1; x1 <= 2; ++x1) for (int x2 = 1; x2 <= 2; ++x2) {
          const double act = row[0] * x0 + row[1] * x1 + row[2] * x2;
          if (act <= rhs + 1e-9) { any = true; best = std::min(best, c[0] * x0 + c[1] * x1 + c[2] * x2); }
        }
        const auto ri = lp::solve_mip(Mi, {});
        expect(static_cast<int>(ri.status) == (any ? 0 : 1), "presolve regression (b) #" + std::to_string(t) + ": status");
        if (any)
          expect(std::abs(ri.objective - best) < 1e-7, "presolve regression (b) #" + std::to_string(t) + ": objective " + std::to_string(ri.objective) + " vs " + std::to_string(best));
      }
      ++tally["presolve regression models"];
    }

    // ---- 5. warm start: bound changes + re-solve must equal a cold solve of the modified model
    {
      Lcg rng{7};
      for (int t = 0; t < 60; ++t) {
        const int n = 8, m = 5;
        std::vector<double> ai(static_cast<std::size_t>(m * n)), c(static_cast<std::size_t>(n)), bi(static_cast<std::size_t>(m));
        for (auto &v : ai) v = rng.uni() < 0.4 ? 0.0 : rng.range(-3, 6);
        for (auto &v : c) v = rng.range(-8, 4);
        for (auto &v : bi) v = rng.range(5, 20);
        auto M = make_model(n, m, 0, ai, {}, c, bi, {}, std::vector<double>(static_cast<std::size_t>(n), 0.0), std::vector<double>(static_cast<std::size_t>(n), 4.0));
        lp::Simplex sx(M);
        auto s0 = sx.solve();
        expect(s0.status == lp::SolveStatus::Optimal, "warm start: base solve");
        for (int k = 0; k < 4; ++k) {
          const int j = rng.range(0, n - 1);
          const double lo = rng.range(0, 2), hi = lo + rng.range(0, 2);
          sx.set_col_bounds(j, lo, hi);
          M.col_lo[static_cast<std::size_t>(j)] = lo;
          M.col_hi[static_cast<std::size_t>(j)] = hi;
          const auto warm = sx.resolve();
          lp::Simplex cold(M);
          const auto fresh = cold.solve();
          expect(warm.status == fresh.status, "warm start #" + std::to_string(t) + ": status differs from cold solve");
          if (fresh.status == lp::SolveStatus::Optimal)
            expect(std::abs(warm.objective - fresh.objective) < 1e-7 * (1 + std::abs(fresh.objective)), "warm start #" + std::to_string(t) + ": objective differs from cold solve");
        }
      }
      ++tally["warm-start re-solves vs cold solves"];
    }

    // ---- 5b. presolve vs no presolve on LPs with eliminable structure (implied-free column singletons, singleton rows, fixed columns)
    {
      Lcg rng{31337};
      for (int t = 0; t < 200; ++t) {
        const int nx = rng.range(3, 7), mrows = rng.range(2, 4), n = nx + mrows + 1, m = mrows + 1;
        std::vector<double> ae(static_cast<std::size_t>(m * n), 0.0), c(static_cast<std::size_t>(n)), lb(static_cast<std::size_t>(n), 0.0), ub(static_cast<std::size_t>(n)), be(static_cast<std::size_t>(m));
        for (int i = 0; i < mrows; ++i) {
          for (int k = 0; k < nx; ++k) ae[static_cast<std::size_t>(i * n + k)] = rng.uni() < 0.6 ? rng.range(-4, 5) : 0.0;
          ae[static_cast<std::size_t>(i * n + nx + i)] = rng.uni() < 0.5 ? 1.0 : -2.0; // singleton column of row i
          be[static_cast<std::size_t>(i)] = rng.range(-6, 6);
        }
        // last row: a singleton row on x0 (folded into a bound) plus the cost structure
        ae[static_cast<std::size_t>(mrows * n + 0)] = rng.range(1, 3);
        be[static_cast<std::size_t>(mrows)] = rng.range(0, 6);
        for (int j = 0; j < n; ++j) {
          c[static_cast<std::size_t>(j)] = rng.range(-5, 5);
          if (j < nx) ub[static_cast<std::size_t>(j)] = rng.range(1, 6);
          else if (j < nx + mrows) { lb[static_cast<std::size_t>(j)] = -INFINITY; ub[static_cast<std::size_t>(j)] = rng.uni() < 0.5 ? INFINITY : 100.0; }
          else { lb[static_cast<std::size_t>(j)] = 2.0; ub[static_cast<std::size_t>(j)] = 2.0; } // fixed column
        }
        const auto M = make_model(n, 0, m, {}, ae, c, {}, be, lb, ub);
        lp::LpOptions off;
        off.presolve.enabled = false;
        const auto ref = lp::solve(M, off), got = lp::solve(M);
        lp::LpOptions primal_only;
        primal_only.want_duals = false; // objective and x come straight from the reduced solve + postsolve of x (no re-solve on the original)
        const auto fast = lp::solve(M, primal_only);
        const std::string tag = "presolve equivalence #" + std::to_string(t);
        expect(ref.status == got.status, tag + ": status " + std::to_string(static_cast<int>(got.status)) + " vs " + std::to_string(static_cast<int>(ref.status)));
        if (ref.status == lp::SolveStatus::Optimal) {
          expect(std::abs(ref.objective - got.objective) < 1e-7 * (1 + std::abs(ref.objective)), tag + ": objective");
          expect(primal_violation(M, got.x, false) < 1e-6, tag + ": primal feasibility of the expanded point");
          double dfeas = 0.0;
          expect(dual_gap(M, got, dfeas) < 1e-6 && dfeas < 1e-6, tag + ": dual certificate after postsolve");
          expect(fast.status == lp::SolveStatus::Optimal && std::abs(fast.objective - ref.objective) < 1e-7 * (1 + std::abs(ref.objective)), tag + ": primal-only objective");
          expect(primal_violation(M, fast.x, false) < 1e-6, tag + ": primal-only postsolved point feasible");
        }
      }
      ++tally["presolve vs no-presolve equivalence LPs"];
    }

    // ---- 5c. doubleton-equation aggregation (aggregate.h): random LPs rich in two-entry equality rows, with and without the stage
    {
      Lcg rng{2468};
      int aggregated = 0, aggregated_long = 0;
      for (int t = 0; t < 300; ++t) {
        const int n = rng.range(5, 10), md = rng.range(2, 5), mg = rng.range(1, 3), m = md + mg;
        std::vector<double> ae(static_cast<std::size_t>(md * n), 0.0), ai(static_cast<std::size_t>(mg * n), 0.0), c(static_cast<std::size_t>(n)), lb(static_cast<std::size_t>(n)), ub(static_cast<std::size_t>(n)), be(static_cast<std::size_t>(md)), bi(static_cast<std::size_t>(mg));
        const bool longer = t % 2 == 1; // odd cases: equality rows of 3-5 entries (implied-free substitution), even: doubletons
        for (int i = 0; i < md; ++i) { // equalities a x_p + b x_q (+ ...) = rhs
          const int entries = longer ? rng.range(3, 5) : 2;
          int first = rng.range(0, n - 1);
          for (int e = 0; e < entries; ++e) {
            const int col = (first + e * 2 + e / 2) % n;
            ae[static_cast<std::size_t>(i * n + col)] = (rng.uni() < 0.5 ? 1.0 : -1.0) * rng.range(1, 3);
          }
          be[static_cast<std::size_t>(i)] = rng.range(-3, 6);
        }
        for (int i = 0; i < mg; ++i) {
          for (int j = 0; j < n; ++j) ai[static_cast<std::size_t>(i * n + j)] = rng.uni() < 0.5 ? rng.range(-3, 4) : 0.0;
          bi[static_cast<std::size_t>(i)] = rng.range(2, 12);
        }
        for (int j = 0; j < n; ++j) {
          c[static_cast<std::size_t>(j)] = rng.range(-5, 5);
          const int kind = rng.range(0, 3);
          lb[static_cast<std::size_t>(j)] = kind == 3 ? -INFINITY : 0.0;
          ub[static_cast<std::size_t>(j)] = kind == 0 ? INFINITY : rng.range(2, 8);
          if (longer && rng.uni() < 0.4) { lb[static_cast<std::size_t>(j)] = -1000.0; ub[static_cast<std::size_t>(j)] = 1000.0; } // wide bounds: usually implied free
        }
        const auto M = make_model(n, mg, md, ai, ae, c, bi, be, lb, ub);
        const auto agg = lp::aggregate(M);
        aggregated += agg.changed();
        if (t % 2 == 1 && agg.changed()) {
          bool long_row = false;
          for (const auto &r : agg.stack) long_row |= r.pool_len > 1;
          aggregated_long += long_row;
        }
        lp::LpOptions off;
        off.presolve.enabled = false;
        lp::LpOptions fast;
        fast.want_duals = false;
        const auto ref = lp::solve(M, off), got = lp::solve(M), quick = lp::solve(M, fast);
        const std::string tag = "aggregation equivalence #" + std::to_string(t);
        const bool ref_decided = ref.status == lp::SolveStatus::Optimal || ref.status == lp::SolveStatus::Infeasible || ref.status == lp::SolveStatus::Unbounded;
        if (!ref_decided)
          continue;
        expect(ref.status == got.status && ref.status == quick.status, tag + ": status " + std::to_string(static_cast<int>(got.status)) + "/" + std::to_string(static_cast<int>(quick.status)) + " vs " + std::to_string(static_cast<int>(ref.status)));
        if (ref.status == lp::SolveStatus::Optimal) {
          expect(std::abs(ref.objective - got.objective) < 1e-7 * (1 + std::abs(ref.objective)), tag + ": objective");
          if (std::getenv("LP_DUMP") && std::abs(ref.objective - quick.objective) >= 1e-7 * (1 + std::abs(ref.objective))) {
            std::fprintf(stderr, "model rows %d cols %d ref %.10g quick %.10g\n", M.rows, M.cols, ref.objective, quick.objective);
            for (int i = 0; i < M.rows; ++i) {
              std::fprintf(stderr, "row %d [%g,%g]:", i, M.row_lo[static_cast<std::size_t>(i)], M.row_hi[static_cast<std::size_t>(i)]);
              for (int j = 0; j < M.cols; ++j)
                for (auto q = M.a.start[static_cast<std::size_t>(j)]; q < M.a.start[static_cast<std::size_t>(j) + 1]; ++q)
                  if (M.a.index[static_cast<std::size_t>(q)] == i) std::fprintf(stderr, " %+g*x%d", M.a.value[static_cast<std::size_t>(q)], j);
              std::fprintf(stderr, "\n");
            }
            for (int j = 0; j < M.cols; ++j) std::fprintf(stderr, "x%d cost %g bounds [%g,%g]\n", j, M.cost[static_cast<std::size_t>(j)], M.col_lo[static_cast<std::size_t>(j)], M.col_hi[static_cast<std::size_t>(j)]);
            std::fprintf(stderr, "stack:");
            for (const auto &r : agg.stack) std::fprintf(stderr, " (row %d col %d len %d)", r.row, r.col, r.pool_len);
            std::fprintf(stderr, "\n");
          }
          expect(std::abs(ref.objective - quick.objective) < 1e-7 * (1 + std::abs(ref.objective)), tag + ": primal-only objective");
          expect(primal_violation(M, quick.x, false) < 1e-6, tag + ": postsolved point feasible");
          double dfeas = 0.0;
          expect(dual_gap(M, got, dfeas) < 1e-6 && dfeas < 1e-6, tag + ": dual certificate");
        }
      }
      expect(aggregated_long > 20, "implied-free substitutions must be exercised (" + std::to_string(aggregated_long) + ")");
      expect(aggregated > 100, "aggregation test must exercise the stage (" + std::to_string(aggregated) + " models aggregated)");
      ++tally["doubleton aggregation equivalence LPs"];
    }

    // ---- 6. MPS reader (free format) on an embedded model with RANGES, negative-upper-bound and integer markers
    {
      std::istringstream in(
          "NAME          TINY\nROWS\n N  OBJ\n G  C1\n L  C2\n E  C3\nCOLUMNS\n"
          "    X         OBJ       1.0   C1        1.0\n    X         C2        1.0   C3        1.0\n"
          "    Y         OBJ       2.0   C1        1.0\n    Y         C2       -1.0   C3        1.0\n"
          "    MARKER    'MARKER'  'INTORG'\n    Z         OBJ       3.0   C3        1.0\n    MARKER    'MARKER'  'INTEND'\n"
          "RHS\n    RHS       C1        3.0   C2        1.0\n    RHS       C3        4.0\n"
          "BOUNDS\n UP BND       X         10.0\n UP BND       Y         10.0\n UP BND       Z         5.0\nENDATA\n");
      const auto mps = lp::read_mps(in);
      const auto &M = mps.model;
      expect(M.rows == 3 && M.cols == 3, "mps: dimensions");
      expect(M.integer.size() == 3 && !M.integer[0] && !M.integer[1] && M.integer[2], "mps: integer markers");
      expect(M.row_lo[0] == 3.0 && std::isinf(M.row_hi[0]) && M.row_hi[1] == 1.0 && std::isinf(M.row_lo[1]) && M.row_lo[2] == 4.0 && M.row_hi[2] == 4.0, "mps: row senses");
      // min x + 2y + 3z : x + y >= 3, x - y <= 1, x + y + z = 4 (z integer)  -> z = 0, x = 2.5, y = 1.5, objective 5.5
      const auto r = lp::solve_mip(M, {});
      expect(r.status == lp::SolveStatus::Optimal && std::abs(r.objective - 5.5) < 1e-9, "mps: solved objective " + std::to_string(r.objective));
      ++tally["mps reader models"];
    }

    // ---- 8. LU: hypersparse and dense triangular solves (m >= 1500 switches ftran to the reach-based path), with eta updates
    {
      Lcg rng{2024};
      const std::int32_t m = 3000;
      std::vector<std::vector<std::pair<std::int32_t, double>>> colsv(static_cast<std::size_t>(m));
      for (std::int32_t j = 0; j < m; ++j) {
        colsv[static_cast<std::size_t>(j)].push_back({j, 4.0 + rng.uni()});
        const int extra = rng.range(0, 3);
        for (int e = 0; e < extra; ++e) {
          const auto i = static_cast<std::int32_t>(rng.range(0, m - 1));
          if (i != j)
            colsv[static_cast<std::size_t>(j)].push_back({i, rng.uni() - 0.5});
        }
      }
      const auto assemble = [&](std::vector<std::int64_t> &st, std::vector<std::int32_t> &ix, std::vector<double> &vl) {
        st.assign(1, 0);
        ix.clear();
        vl.clear();
        for (auto &c : colsv) {
          std::sort(c.begin(), c.end());
          std::vector<std::pair<std::int32_t, double>> merged;
          for (const auto &e : c) {
            if (!merged.empty() && merged.back().first == e.first) merged.back().second += e.second; else merged.push_back(e);
          }
          c = merged;
          for (const auto &e : c) { ix.push_back(e.first); vl.push_back(e.second); }
          st.push_back(static_cast<std::int64_t>(ix.size()));
        }
      };
      std::vector<std::int64_t> st;
      std::vector<std::int32_t> ix;
      std::vector<double> vl;
      assemble(st, ix, vl);
      lp::BasisFactor f;
      expect(f.factorize(m, st, ix, vl).empty(), "lu: factorization of a diagonally dominant matrix");
      const auto check = [&](const std::string &tag) {
        for (int t = 0; t < 40; ++t) {
          const int nz = t % 4 == 0 ? m : rng.range(1, 6); // dense, then very sparse right-hand sides
          std::vector<double> b(static_cast<std::size_t>(m), 0.0), x;
          for (int k = 0; k < nz; ++k) b[static_cast<std::size_t>(nz == m ? k : rng.range(0, m - 1))] = rng.uni() - 0.5;
          x = b;
          f.ftran(x);
          std::vector<double> r = b;
          for (std::int32_t j = 0; j < m; ++j)
            for (const auto &e : colsv[static_cast<std::size_t>(j)]) r[static_cast<std::size_t>(e.first)] -= e.second * x[static_cast<std::size_t>(j)];
          double res = 0.0;
          for (double v : r) res = std::max(res, std::abs(v));
          expect(res < 1e-9, tag + ": ftran residual " + std::to_string(res));
          std::vector<double> y = b;
          f.btran(y);
          std::vector<double> rt = b;
          for (std::int32_t j = 0; j < m; ++j) {
            double dotv = 0.0;
            for (const auto &e : colsv[static_cast<std::size_t>(j)]) dotv += e.second * y[static_cast<std::size_t>(e.first)];
            rt[static_cast<std::size_t>(j)] -= dotv;
          }
          res = 0.0;
          for (double v : rt) res = std::max(res, std::abs(v));
          expect(res < 1e-9, tag + ": btran residual " + std::to_string(res));
        }
      };
      check("fresh factor");
      for (int u = 0; u < 12; ++u) { // product-form updates: replace a column, keep the factor current
        const auto r = static_cast<std::int32_t>(rng.range(0, m - 1));
        std::vector<std::pair<std::int32_t, double>> nc{{r, 5.0 + rng.uni()}};
        for (int e = 0; e < 3; ++e) {
          const auto i = static_cast<std::int32_t>(rng.range(0, m - 1));
          if (i != r) nc.push_back({i, rng.uni() - 0.5});
        }
        std::sort(nc.begin(), nc.end());
        nc.erase(std::unique(nc.begin(), nc.end(), [](auto &a, auto &b) { return a.first == b.first; }), nc.end());
        std::vector<double> alpha(static_cast<std::size_t>(m), 0.0);
        for (const auto &e : nc) alpha[static_cast<std::size_t>(e.first)] = e.second;
        f.ftran(alpha);
        if (std::abs(alpha[static_cast<std::size_t>(r)]) < 1e-3)
          continue;
        f.update(r, alpha);
        colsv[static_cast<std::size_t>(r)] = nc;
      }
      check("after eta updates");
      ++tally["lu hypersparse/dense solves"];
    }

    // ---- 7. Series bridge: the dense milp::Problem of every reference case solved by the sparse stack
    {
      int idx = 0;
      for (const auto &cs : milp_reference::cases()) {
        const std::string tag = std::string("bridge ") + cs.kind + " #" + std::to_string(idx++);
        const auto F64 = pluss::ScalarType::Float64;
        pluss::subsp::math::milp::Problem p;
        p.lp.c = pluss::make_series(cs.c, {cs.n}).to(F64);
        if (cs.mi) { p.lp.a_ub = pluss::make_series(cs.ai, {cs.mi, cs.n}).to(F64); p.lp.b_ub = pluss::make_series(cs.bi, {cs.mi}).to(F64); }
        if (cs.me) { p.lp.a_eq = pluss::make_series(cs.ae, {cs.me, cs.n}).to(F64); p.lp.b_eq = pluss::make_series(cs.be, {cs.me}).to(F64); }
        p.lp.lower = pluss::make_series(cs.lb, {cs.n}).to(F64);
        p.lp.upper = pluss::make_series(cs.ub, {cs.n}).to(F64);
        p.integer = pluss::make_series(cs.integer, {cs.n}).to(F64);
        const auto r = lp::solve_sparse(p);
        expect(static_cast<int>(r.status) == cs.status, tag + ": status");
        if (cs.status == 0)
          expect(std::abs(r.objective - cs.objective) < 1e-6 * (1 + std::abs(cs.objective)), tag + ": objective");
      }
      ++tally["series-bridge milp cases"];
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  for (const auto &kv : tally)
    std::cout << kv.first << ": " << kv.second << " certified\n";
  std::cout << "sparse lp/milp tests passed\n";
  return 0;
}
