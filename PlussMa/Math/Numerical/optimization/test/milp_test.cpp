// Verification of pluss::subsp::math::milp (batched branch-and-bound) against HiGHS milp (gap 0) and exhaustive
// enumeration. Every incumbent is re-checked against the ORIGINAL data: feasibility, integrality, objective value.
#include "milp_reference.h"
#include "pluss/subsp/math/milp.h"

#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace mp = pluss::subsp::math::milp;
using pluss::Series;

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
} // namespace

int main() {
  std::map<std::string, int> tally;
  int total_nodes = 0, max_nodes = 0;
  try {
    for (const auto method : {pluss::subsp::math::linprog::Method::InteriorPoint, pluss::subsp::math::linprog::Method::Simplex}) {
    int idx = 0;
    for (const auto &cs : milp_reference::cases()) {
      const std::string tag = std::string(method == pluss::subsp::math::linprog::Method::Simplex ? "[simplex] " : "[interior point] ") + cs.kind + " #" + std::to_string(idx++);
      const int n = cs.n;
      mp::Problem p;
      p.lp.c = mk(cs.c, {n});
      if (cs.mi) {
        p.lp.a_ub = mk(cs.ai, {cs.mi, n});
        p.lp.b_ub = mk(cs.bi, {cs.mi});
      }
      if (cs.me) {
        p.lp.a_eq = mk(cs.ae, {cs.me, n});
        p.lp.b_eq = mk(cs.be, {cs.me});
      }
      p.lp.lower = mk(cs.lb, {n});
      p.lp.upper = mk(cs.ub, {n});
      p.integer = mk(cs.integer, {n});
      mp::Options mopt;
      mopt.lp.method = method;
      const auto r = mp::solve(p, mopt);
      total_nodes += r.nodes;
      max_nodes = std::max(max_nodes, r.nodes);
      const int st = static_cast<int>(r.status);
      expect(st == cs.status, tag + ": status " + std::to_string(st) + " expected " + std::to_string(cs.status));
      if (st == 0) {
        const auto x = r.x.to_vec<double>();
        for (int i = 0; i < cs.mi; ++i)
          expect(dot(&cs.ai[static_cast<std::size_t>(i * n)], x, n) <= cs.bi[static_cast<std::size_t>(i)] + 1e-5, tag + ": inequality violated");
        for (int i = 0; i < cs.me; ++i)
          expect(std::abs(dot(&cs.ae[static_cast<std::size_t>(i * n)], x, n) - cs.be[static_cast<std::size_t>(i)]) <= 1e-5, tag + ": equality violated");
        for (int j = 0; j < n; ++j) {
          expect(x[static_cast<std::size_t>(j)] >= cs.lb[static_cast<std::size_t>(j)] - 1e-6 && x[static_cast<std::size_t>(j)] <= cs.ub[static_cast<std::size_t>(j)] + 1e-6, tag + ": bound violated");
          if (cs.integer[static_cast<std::size_t>(j)] != 0.0)
            expect(x[static_cast<std::size_t>(j)] == std::round(x[static_cast<std::size_t>(j)]), tag + ": non-integral value");
        }
        const double val = dot(cs.c.data(), x, n);
        expect(std::abs(val - cs.objective) < 1e-6 * (1 + std::abs(cs.objective)), tag + ": objective " + std::to_string(val) + " vs HiGHS " + std::to_string(cs.objective));
        expect(std::abs(r.objective.item<double>() - val) < 1e-7 * (1 + std::abs(val)), tag + ": reported objective inconsistent");
      }
      ++tally[std::string(method == pluss::subsp::math::linprog::Method::Simplex ? "simplex " : "interior-point ") + (st == 0 ? "optimal" : st == 1 ? "infeasible" : "unbounded")];
    }
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  for (const auto &kv : tally)
    std::cout << kv.first << ": " << kv.second << " problems certified\n";
  std::cout << "branch-and-bound nodes: total " << total_nodes << ", max " << max_nodes << "\nmilp tests passed\n";
  return 0;
}
