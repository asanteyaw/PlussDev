// Regression: sentinel infinities (1e30, 1e300, ...) in variable bounds must behave exactly like real infinities in every LP/MILP entry point
// (dense linprog, sparse simplex pipeline, interior point, dense milp, sparse branch-and-cut). Before the fix a 1e30 bound made the dense solver
// report "infeasible", 1e300 gave NaN, and the sparse solver returned a wrong optimum.
#include "pluss/subsp/math/linprog.h"
#include "pluss/subsp/math/lp/ipm.h"
#include "pluss/subsp/math/lp/mip.h"
#include "pluss/subsp/math/lp/series_bridge.h"
#include "pluss/subsp/math/milp.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace lp = pluss::subsp::math::linprog;
namespace sp = pluss::subsp::math::lp;
namespace mi = pluss::subsp::math::milp;

namespace {
pluss::Series v(std::vector<double> x, std::vector<std::int64_t> s) { return pluss::make_series(x, s).to(pluss::ScalarType::Float64); }
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
} // namespace

int main() {
  try {
    const double inf = std::numeric_limits<double>::infinity();
    for (const double big : {inf, 1e20, 1e30, 1e300}) {
      const std::string tag = " (bound " + std::to_string(big) + ")";
      // max x + y  s.t. x <= 1, y <= 2, x + y <= 2.5  with free variables: optimum 2.5
      lp::Problem p;
      p.c = v({-1, -1}, {2});
      p.a_ub = v({1, 0, 0, 1, 1, 1}, {3, 2});
      p.b_ub = v({1, 2, 2.5}, {3});
      p.lower = v({-big, -big}, {2});
      p.upper = v({big, big}, {2});
      const auto d = lp::solve(p, {});
      expect(d.status.item<double>() == 0.0 && std::abs(d.objective.item<double>() + 2.5) < 1e-9, "dense linprog" + tag);
      const auto s = sp::solve_sparse(p);
      expect(s.status == sp::SolveStatus::Optimal && std::abs(s.objective + 2.5) < 1e-9, "sparse simplex pipeline" + tag);
      sp::Model m = sp::to_sparse(p);
      const auto ipm = sp::solve_ipm(m);
      expect(ipm.status == sp::SolveStatus::Optimal && std::abs(ipm.objective + 2.5) < 1e-6, "interior point" + tag);
      // a Model filled by hand (never touched by the bridge) with sentinel bounds and sentinel row sides
      sp::Model raw = sp::to_sparse(p);
      raw.col_lo.assign(2, -big);
      raw.col_hi.assign(2, big);
      raw.row_lo.assign(raw.rows, -big);
      const auto r = sp::solve(raw);
      expect(r.status == sp::SolveStatus::Optimal && std::abs(r.objective + 2.5) < 1e-9, "hand-built sparse model with sentinel row sides" + tag);
      // MILP: max x + y, x, y integer in [0, big?] -> bounded by rows; x <= 1.5, y <= 2.5, x + y <= 3 -> optimum 3 with integer solution
      mi::Problem q;
      q.lp.c = v({-1, -1}, {2});
      q.lp.a_ub = v({1, 0, 0, 1, 1, 1}, {3, 2});
      q.lp.b_ub = v({1.5, 2.5, 3.0}, {3});
      q.lp.lower = v({0, 0}, {2});
      q.lp.upper = v({big, big}, {2});
      q.integer = v({0, 0}, {2}); // continuous first; integer variables with an infinite/sentinel bound are checked below
      const auto mr = mi::solve(q, {});
      expect(std::abs(mr.objective.item<double>() + 3.0) < 1e-9, "dense milp with continuous variables" + tag);
      mi::Problem qi = q;
      qi.integer = v({1, 1}, {2});
      bool threw = false;
      try { (void)mi::solve(qi, {}); } catch (const std::invalid_argument &) { threw = true; }
      expect(threw, "dense milp: an integer variable with an infinite/sentinel bound raises" + tag);
      sp::Model mm = sp::to_sparse(q);
      mm.integer.assign(2, 1);
      mm.col_hi.assign(2, big);
      const auto sm = sp::solve_mip(mm);
      expect(sm.status == sp::SolveStatus::Optimal && std::abs(sm.objective + 3.0) < 1e-9, "sparse branch-and-cut with sentinel bounds (rows bound the integers)" + tag);
    }
    // a vacuous constraint written with a sentinel right-hand side is rejected loudly by the dense solver
    {
      lp::Problem p;
      p.c = v({-1}, {1});
      p.a_ub = v({1, 1}, {2, 1});
      p.b_ub = v({1.0, 1e30}, {2});
      p.lower = v({0.0}, {1});
      p.upper = v({10.0}, {1});
      bool threw = false;
      try { (void)lp::solve(p, {}); } catch (const std::invalid_argument &) { threw = true; }
      expect(threw, "dense linprog rejects a right-hand side beyond 1e20");
    }
    // a variable with no `lower` is free in the dense solver and in the bridge alike: minimize x0 - x1 with 2 <= x0 + x1 <= 4
    // is unbounded when x is free (x0 = -t, x1 = 4 + t), and optimal at (0, 4) when x >= 0 is stated
    {
      lp::Problem p;
      p.c = v({1, -1}, {2});
      p.a_ub = v({-1, -1, 1, 1}, {2, 2});
      p.b_ub = v({-2, 4}, {2});
      expect(lp::solve(p, {}).status.item<double>() == 2.0, "dense linprog: omitted lower bounds mean free (unbounded)");
      expect(sp::solve_sparse(p).status == sp::SolveStatus::Unbounded, "bridge: omitted lower bounds mean free (unbounded)");
      p.lower = v({0, 0}, {2});
      expect(std::abs(lp::solve(p, {}).objective.item<double>() + 4.0) < 1e-9, "dense linprog with lower = 0");
      expect(std::abs(sp::solve_sparse(p).objective + 4.0) < 1e-9, "bridge with lower = 0");
    }
    std::cout << "linprog infinity tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
