// Pinned objective floors (flux variability at fraction 1) leave the interior-point method almost no interior: lanes stall at
// IterationLimit / NearOptimal. With Options::polish (default) those lanes are re-solved by the exact simplex and must agree with it.
#include "pluss/subsp/math/linprog.h"
#include "pluss/subsp/natsci/biology/metabolism/fba.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace mt = pluss::subsp::natsci::biology::metabolism;
namespace lp = pluss::subsp::math::linprog;
using pluss::Series;

namespace {
Series mk(const std::vector<double> &v, std::vector<std::int64_t> s) { return pluss::make_series(v, s).to(pluss::ScalarType::Float64); }
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
} // namespace

int main() {
  try {
    std::mt19937 g(5);
    std::uniform_real_distribution<double> U(0, 1);
    for (int trial = 0; trial < 2; ++trial) {
      const int m = 40 + 20 * trial, r = 2 * m + 20;
      std::vector<double> S(static_cast<std::size_t>(m * r), 0.0), v0(static_cast<std::size_t>(r));
      for (auto &x : v0) x = 0.5 + U(g);
      for (int i = 0; i < m; ++i) { // random sparse network with a known strictly positive steady flux (feasible, highly degenerate)
        double acc = 0;
        for (int j = 0; j < r - 1; ++j)
          if (U(g) < 0.12) { const double s = U(g) < 0.5 ? -1 : 1; S[static_cast<std::size_t>(i * r + j)] = s; acc += s * v0[static_cast<std::size_t>(j)]; }
        S[static_cast<std::size_t>(i * r + r - 1)] = -acc / v0[static_cast<std::size_t>(r - 1)];
      }
      mt::Network net;
      net.stoichiometry = mk(S, {m, r});
      net.lower = mk(std::vector<double>(static_cast<std::size_t>(r), 0.0), {r});
      net.upper = mk(std::vector<double>(static_cast<std::size_t>(r), 10.0), {r});
      std::vector<double> c(static_cast<std::size_t>(r), 0.0);
      for (int j = 0; j < 5; ++j) c[static_cast<std::size_t>(j)] = 1.0;
      const Series obj = mk(c, {r});
      const auto exact = mt::flux_variability(net, obj, 1.0); // simplex
      lp::Options ipm; // polish on by default
      expect(ipm.polish && ipm.method == lp::Method::InteriorPoint, "defaults: interior point with polish");
      const auto polished = mt::flux_variability(net, obj, 1.0, ipm);
      const auto a = exact.minimum.to_vec<double>(), b = polished.minimum.to_vec<double>(), a2 = exact.maximum.to_vec<double>(), b2 = polished.maximum.to_vec<double>();
      double worst = 0.0;
      for (int j = 0; j < r; ++j)
        worst = std::max({worst, std::abs(a[static_cast<std::size_t>(j)] - b[static_cast<std::size_t>(j)]), std::abs(a2[static_cast<std::size_t>(j)] - b2[static_cast<std::size_t>(j)])});
      expect(worst < 1e-7, "polished interior point matches the simplex on pinned-floor FVA, worst " + std::to_string(worst));
      // without polishing the same lanes do not all reach optimality (documents why the polish exists; deterministic for this seed)
      lp::Options raw = ipm;
      raw.polish = false;
      bool threw = false;
      try { (void)mt::flux_variability(net, obj, 1.0, raw); } catch (const std::exception &) { threw = true; }
      if (trial == 0)
        expect(threw, "without polish the pinned-floor LPs do not all reach optimality");
    }
    // infeasible / unbounded verdicts of the interior point are untouched by polishing
    {
      lp::Problem p;
      p.c = mk({-1, 0}, {2});
      p.a_ub = mk({1, 1}, {1, 2});
      p.b_ub = mk({-1}, {1});
      p.lower = mk({0, 0}, {2});
      p.upper = mk({5, 5}, {2});
      expect(lp::solve(p, {}).status.item<double>() == static_cast<double>(lp::Status::PrimalInfeasible), "infeasible stays infeasible");
      p.b_ub = mk({1}, {1});
      p.upper = mk({std::numeric_limits<double>::infinity(), 5}, {2});
      p.a_ub = mk({0, 1}, {1, 2});
      expect(lp::solve(p, {}).status.item<double>() == static_cast<double>(lp::Status::Unbounded), "unbounded stays unbounded");
    }
    std::cout << "linprog polish tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
