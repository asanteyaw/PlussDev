<!-- generated from how_a_search_ends.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# How a search ends

[Back to Choosing Whole Things](../milp_01_integer_programs.md)


Infeasible, optimal, and a search cut off by a node limit.

```cpp
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
```

Output

```text
2 x = 1, x integer:   Infeasible

node limit      status      nodes   best found    best bound    gap
20              NodeLimit   31      660.000       666.587       6.587
200             NodeLimit   209     665.000       666.299       1.299
200000          Optimal     273     665.000       665.000       0.000
```
The program `2x = 1` with `x` an integer is `Infeasible`: its relaxation is feasible at `x = 0.5`, and no whole number works. The second part is a 24-item knapsack whose payoffs are close to the costs, which leaves many near-ties to prove. With a limit of 20 nodes the search stops with `NodeLimit` having found a solution worth 660 and proved that none exceeds 666.587: the answer is within 6.587 of the best possible. With 200 nodes it has 665 and the bound 666.299, a gap of 1.299. Allowed to finish, it proves 665 optimal in 273 nodes with a gap of 0. The node counts can exceed the limit a little because nodes are solved in batches.

A node limit gives an *anytime* answer: the best solution so far, with a proof of how far it can be from optimal. When the gap is small enough for your purpose, stop there; when it is not, raise the limit. The best bound is reported as the objective of the minimization, so with a negated payoff a *larger* bound means a *weaker* guarantee, which the program prints as a positive gap.
