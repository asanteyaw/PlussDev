// Verification of pluss::subsp::math::linprog against HiGHS (SciPy) and published NETLIB optima.
// Every returned answer is also CERTIFIED against the original problem data, independently of the solver:
//   optimal     -> primal feasibility, duality gap (objective vs dual objective), objective vs HiGHS / NETLIB
//   infeasible  -> Farkas certificate in the original constraint space (y_eq free, y_ub <= 0, y_u <= 0, ...)
//   unbounded   -> recession ray: A_ub d <= 0, A_eq d = 0, respects bounds, c^T d < 0
#include "linprog_reference.h"
#include "pluss/subsp/math/linprog.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace lp = pluss::subsp::math::linprog;
using pluss::Series;
using linprog_reference::Group;

namespace {
const auto F64 = pluss::ScalarType::Float64;
Series mk(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
double dot(const double *a, const std::vector<double> &x, int n) {
  double s = 0;
  for (int j = 0; j < n; ++j)
    s += a[j] * x[static_cast<std::size_t>(j)];
  return s;
}

void check_group(const Group &g, std::map<std::string, int> &tally, lp::Method method) {
  const int n = g.n, L = static_cast<int>(g.lanes.size());
  lp::Problem p;
  std::vector<double> C, BI, BE, LB, UB;
  for (const auto &l : g.lanes) {
    C.insert(C.end(), l.c.begin(), l.c.end());
    BI.insert(BI.end(), l.bi.begin(), l.bi.end());
    BE.insert(BE.end(), l.be.begin(), l.be.end());
    LB.insert(LB.end(), l.lb.begin(), l.lb.end());
    UB.insert(UB.end(), l.ub.begin(), l.ub.end());
  }
  p.c = mk(C, {L, n});
  if (g.mi) {
    p.a_ub = mk(g.ai, {g.mi, n});
    p.b_ub = mk(BI, {L, g.mi});
  }
  if (g.me) {
    p.a_eq = mk(g.ae, {g.me, n});
    p.b_eq = mk(BE, {L, g.me});
  }
  p.lower = mk(LB, {L, n});
  p.upper = mk(UB, {L, n});
  lp::Options opts;
  opts.method = method;
  const auto r = lp::solve(p, opts);
  const auto st = r.status.to_vec<double>(), obj = r.objective.to_vec<double>(), dual = r.dual_objective.to_vec<double>();
  const auto xs = r.x.to_vec<double>(), ray = r.ray.to_vec<double>(), fk = r.farkas.to_vec<double>();
  const std::size_t rows = fk.size() / static_cast<std::size_t>(L);
  for (int li = 0; li < L; ++li) {
    const auto &lane = g.lanes[static_cast<std::size_t>(li)];
    const std::string tag = std::string(g.name) + " lane " + std::to_string(li);
    int status = static_cast<int>(st[static_cast<std::size_t>(li)]);
    const bool near = status == 4;                      // NearOptimal: accepted as optimal, certified below at 1e-6
    if (near) {
      ++tally[method == lp::Method::Simplex ? "simplex near-optimal (unexpected)" : "interior-point near-optimal (accuracy-limited, certified)"];
      status = 0;
    }
    expect(status == lane.status, tag + ": status " + std::to_string(status) + " expected " + std::to_string(lane.status));
    std::vector<double> x(xs.begin() + li * n, xs.begin() + (li + 1) * n);
    if (status == 0) {
      double bscale = 1.0, viol = 0.0;
      for (int i = 0; i < g.mi; ++i) {
        viol = std::max(viol, dot(&g.ai[static_cast<std::size_t>(i * n)], x, n) - lane.bi[static_cast<std::size_t>(i)]);
        bscale = std::max(bscale, std::abs(lane.bi[static_cast<std::size_t>(i)]));
      }
      for (int i = 0; i < g.me; ++i) {
        viol = std::max(viol, std::abs(dot(&g.ae[static_cast<std::size_t>(i * n)], x, n) - lane.be[static_cast<std::size_t>(i)]));
        bscale = std::max(bscale, std::abs(lane.be[static_cast<std::size_t>(i)]));
      }
      for (int j = 0; j < n; ++j)
        viol = std::max({viol, lane.lb[static_cast<std::size_t>(j)] - x[static_cast<std::size_t>(j)], x[static_cast<std::size_t>(j)] - lane.ub[static_cast<std::size_t>(j)]});
      const double o = obj[static_cast<std::size_t>(li)], d = dual[static_cast<std::size_t>(li)];
      expect(viol / bscale < 1e-6, tag + ": primal infeasibility " + std::to_string(viol));
      expect(std::abs(o - lane.objective) < 1e-6 * (1 + std::abs(lane.objective)), tag + ": objective " + std::to_string(o) + " vs HiGHS " + std::to_string(lane.objective));
      expect(std::abs(o - d) < 1e-6 * (1 + std::abs(o)), tag + ": duality gap " + std::to_string(o - d));
      if (!std::isnan(g.published))
        expect(std::abs(o - g.published) < 1e-6 * (1 + std::abs(g.published)), tag + ": objective vs published NETLIB value");
    } else if (status == 1) {
      // Farkas certificate in the ORIGINAL space: y = (y_eq free, y_ub <= 0), g = Aeq^T y_eq + Aub^T y_ub.
      // No feasible x can exist if  sup_{l <= x <= u} g^T x  <  beq^T y_eq + bub^T y_ub.
      expect(rows == static_cast<std::size_t>(g.me + g.mi), tag + ": certificate size");
      const double *y = &fk[static_cast<std::size_t>(li) * rows];
      std::vector<double> gcol(static_cast<std::size_t>(n), 0.0);
      for (int i = 0; i < g.me; ++i)
        for (int j = 0; j < n; ++j)
          gcol[static_cast<std::size_t>(j)] += g.ae[static_cast<std::size_t>(i * n + j)] * y[i];
      for (int i = 0; i < g.mi; ++i)
        for (int j = 0; j < n; ++j)
          gcol[static_cast<std::size_t>(j)] += g.ai[static_cast<std::size_t>(i * n + j)] * y[g.me + i];
      double rhs = 0.0, sup = 0.0, leak = 0.0, wrong_sign = 0.0;
      for (int i = 0; i < g.me; ++i)
        rhs += y[i] * lane.be[static_cast<std::size_t>(i)];
      for (int i = 0; i < g.mi; ++i) {
        rhs += y[g.me + i] * lane.bi[static_cast<std::size_t>(i)];
        wrong_sign = std::max(wrong_sign, y[g.me + i]);
      }
      for (int j = 0; j < n; ++j) {
        const double gj = gcol[static_cast<std::size_t>(j)];
        const double lj = lane.lb[static_cast<std::size_t>(j)], uj = lane.ub[static_cast<std::size_t>(j)];
        if (gj > 0.0)
          (std::isfinite(uj) ? sup += gj * uj : leak = std::max(leak, gj));
        else if (gj < 0.0)
          (std::isfinite(lj) ? sup += gj * lj : leak = std::max(leak, -gj));
      }
      const double gap = rhs - sup;
      expect(gap > 0.0 && leak <= 1e-6 * gap && wrong_sign <= 1e-6 * gap,
             tag + ": Farkas certificate invalid (gap " + std::to_string(gap) + ", unbounded-side leak " + std::to_string(leak) + ")");
    } else if (status == 2) {
      std::vector<double> d(ray.begin() + li * n, ray.begin() + (li + 1) * n);
      double dn = 1e-300;
      for (double v : d)
        dn = std::max(dn, std::abs(v));
      for (int i = 0; i < g.mi; ++i)
        expect(dot(&g.ai[static_cast<std::size_t>(i * n)], d, n) <= 1e-6 * dn, tag + ": ray violates A_ub d <= 0");
      for (int i = 0; i < g.me; ++i)
        expect(std::abs(dot(&g.ae[static_cast<std::size_t>(i * n)], d, n)) <= 1e-6 * dn, tag + ": ray violates A_eq d = 0");
      for (int j = 0; j < n; ++j) {
        if (std::isfinite(lane.lb[static_cast<std::size_t>(j)]))
          expect(d[static_cast<std::size_t>(j)] >= -1e-6 * dn, tag + ": ray leaves the lower bound");
        if (std::isfinite(lane.ub[static_cast<std::size_t>(j)]))
          expect(d[static_cast<std::size_t>(j)] <= 1e-6 * dn, tag + ": ray leaves the upper bound");
      }
      double cd = 0;
      for (int j = 0; j < n; ++j)
        cd += lane.c[static_cast<std::size_t>(j)] * d[static_cast<std::size_t>(j)];
      expect(cd < -1e-9 * dn, tag + ": ray does not decrease the objective");
    }
    ++tally[std::string(method == lp::Method::Simplex ? "simplex " : "interior-point ") + (status == 0 ? "optimal" : status == 1 ? "infeasible" : "unbounded")];
  }
}

// Warm start: re-solve cost-perturbed and bound-tightened variants of every optimal reference lane from its own optimal basis.
// Cross-checks warm simplex = cold simplex = interior point (three independent paths) and counts the pivots saved.
void check_warm_start() {
  int variants = 0, warm_pivots = 0, cold_pivots = 0;
  for (const auto &g : linprog_reference::groups()) {
    if (g.lanes.size() != 1 || g.lanes[0].status != 0)
      continue;
    const int n = g.n;
    const auto &l = g.lanes[0];
    auto build = [&](const std::vector<double> &c, const std::vector<double> &ub) {
      lp::Problem p;
      p.c = mk(c, {n});
      if (g.mi) {
        p.a_ub = mk(g.ai, {g.mi, n});
        p.b_ub = mk(l.bi, {g.mi});
      }
      if (g.me) {
        p.a_eq = mk(g.ae, {g.me, n});
        p.b_eq = mk(l.be, {g.me});
      }
      p.lower = mk(l.lb, {n});
      p.upper = mk(ub, {n});
      return p;
    };
    lp::Options simplex;
    simplex.method = lp::Method::Simplex;
    const auto base = lp::solve(build(l.c, l.ub), simplex);
    if (base.basis.empty() || base.basis[0].empty())
      continue;
    for (int kind = 0; kind < 2; ++kind) {
      std::vector<double> c = l.c, ub = l.ub;
      for (int j = 0; j < n; ++j) {
        if (kind == 0)
          c[static_cast<std::size_t>(j)] += 0.3 * std::sin(1.7 * (j + 1) + g.n); // cost change: primal pivots
        else if (std::isfinite(ub[static_cast<std::size_t>(j)]) && std::isfinite(l.lb[static_cast<std::size_t>(j)]) && j % 3 == 0)
          ub[static_cast<std::size_t>(j)] = std::max(l.lb[static_cast<std::size_t>(j)] + 0.05, 0.5 * (ub[static_cast<std::size_t>(j)] + l.lb[static_cast<std::size_t>(j)])); // bound change: dual pivots
      }
      auto p = build(c, ub);
      const auto cold = lp::solve(p, simplex);
      auto pw = p;
      pw.warm_basis = base.basis[0];
      pw.warm_at_upper = base.at_upper[0];
      const auto warm = lp::solve(pw, simplex);
      const auto ipm = lp::solve(p, lp::Options{});
      const std::string tag = std::string(g.name) + " variant " + std::to_string(kind);
      expect(!warm.warm_started.empty() && warm.warm_started[0] == 1, tag + ": the warm start did not engage (silent cold start)");
      expect(warm.status.item<double>() == cold.status.item<double>(), tag + ": warm / cold status differ");
      if (cold.status.item<double>() == 0.0) {
        expect(std::abs(warm.objective.item<double>() - cold.objective.item<double>()) < 1e-9 * (1 + std::abs(cold.objective.item<double>())), tag + ": warm objective != cold objective");
        if (ipm.status.item<double>() == 0.0 || ipm.status.item<double>() == 4.0)
          expect(std::abs(ipm.objective.item<double>() - cold.objective.item<double>()) < 1e-6 * (1 + std::abs(cold.objective.item<double>())), tag + ": simplex objective != interior-point objective");
        warm_pivots += warm.iterations;
        cold_pivots += cold.iterations;
      }
      ++variants;
    }
  }
  std::cout << "warm start: " << variants << " variants agree with cold simplex and interior point; last-lane pivots warm " << warm_pivots << " vs cold " << cold_pivots << "\n";
}
} // namespace

int main() {
  std::map<std::string, int> tally;
  try {
    for (const auto method : {lp::Method::InteriorPoint, lp::Method::Simplex})
      for (const auto &g : linprog_reference::groups())
        check_group(g, tally, method);
    check_warm_start();
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  for (const auto &kv : tally)
    std::cout << kv.first << ": " << kv.second << " problems certified\n";
  std::cout << "linprog tests passed\n";
  return 0;
}
