// The ways an integer search can end. Optimal: a solution proven best within the gap tolerance. Infeasible: no integer point
// satisfies the constraints (here 2 x = 1 with x integer, whose relaxation is feasible at x = 0.5). NodeLimit: the search was cut
// off after `max_nodes` nodes, and the result still reports the best solution found and the best bound proved, so the gap between
// them says how far from optimal it might be. The harder program is a knapsack with 24 items whose payoffs are close to their
// costs, which gives the search many near-ties to prove.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

const char* name_of(milp::Status s) {
  switch (s) {
    case milp::Status::Optimal: return "Optimal";
    case milp::Status::Infeasible: return "Infeasible";
    case milp::Status::Unbounded: return "Unbounded";
    case milp::Status::NodeLimit: return "NodeLimit";
  }
  return "?";
}

int main() {
  std::cout << std::fixed << std::setprecision(3);
  {
    milp::Problem p;  // 2 x = 1 with x an integer in [0, 5]
    p.lp.c = vec({1});
    p.lp.a_eq = mat({2}, 1, 1);
    p.lp.b_eq = vec({1});
    p.lp.lower = vec({0});
    p.lp.upper = vec({5});
    p.integer = vec({1});
    const auto r = milp::solve(p);
    std::cout << "2 x = 1, x integer:   " << name_of(r.status) << "\n\n";
  }

  const int n = 24;
  std::vector<double> cost(n), payoff(n);
  std::uint64_t state = 5;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  double total = 0.0;
  for (int j = 0; j < n; ++j) {
    cost[static_cast<std::size_t>(j)] = 20.0 + std::floor(60.0 * uniform());
    payoff[static_cast<std::size_t>(j)] = -(cost[static_cast<std::size_t>(j)] + 5.0 + std::floor(3.0 * uniform()));  // negated payoff
    total += cost[static_cast<std::size_t>(j)];
  }
  milp::Problem hard;
  hard.lp.c = vec(payoff);
  hard.lp.a_ub = mat(cost, 1, n);
  hard.lp.b_ub = vec({std::floor(total / 2.0)});
  hard.lp.lower = vec(std::vector<double>(n, 0.0));
  hard.lp.upper = vec(std::vector<double>(n, 1.0));
  hard.integer = vec(std::vector<double>(n, 1.0));

  std::cout << std::left << std::setw(16) << "node limit" << std::setw(12) << "status" << std::setw(8) << "nodes" << std::setw(14) << "best found" << std::setw(14) << "best bound" << "gap\n";
  for (const int limit : {20, 200, 200000}) {
    milp::Options options;
    options.max_nodes = limit;
    const auto r = milp::solve(hard, options);
    std::cout << std::left << std::setw(16) << limit << std::setw(12) << name_of(r.status) << std::setw(8) << r.nodes << std::setw(14) << -r.objective.item<double>()
              << std::setw(14) << -r.best_bound << -r.best_bound - -r.objective.item<double>() << "\n";
  }
  return 0;
}
