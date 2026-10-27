<!-- generated from choosing_whole_things.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Choosing whole things

[Back to Integer Programming 1: Choosing Whole Things](../milp_01_integer_programs.md)

Some decisions come in whole units: a project is taken or not, a machine is bought or not, a truck carries a whole number of pallets. An **integer program** is a linear program in which some variables must take whole-number values. The extra requirement is what makes these problems hard, and what makes them useful: nearly every real plan with an on/off choice is one. The solver is a branch-and-bound search that solves the linear **relaxation** (with fractions allowed), splits on a fractional variable, and discards branches whose relaxation cannot beat the best whole solution found.

Three programs. A ten-project **knapsack** is solved and checked against a brute-force search of all 1024 subsets; the relaxation shows the best payoff fractions would allow. **General integers** that are not 0 or 1 show when the relaxation is already whole (the search finishes at the root) and when it is not. And the **endings**: infeasible, optimal, and a search cut off by a node limit, which gives an *anytime* answer with a proof of how far it can be from optimal.

## Program 1: Choosing projects under a budget

Ten projects each have a cost and a payoff, and a budget of 40 is available. Choose the projects, each taken whole or not at all, to maximize the total payoff. The solver minimizes, so the payoff is negated.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  const std::vector<double> cost = {12, 7, 11, 8, 9, 6, 14, 5, 10, 13};
  const std::vector<double> payoff = {24, 13, 23, 15, 16, 11, 28, 8, 19, 25};
  const double budget = 40.0;
  const int n = static_cast<int>(cost.size());

  milp::Problem problem;
  std::vector<double> negated;
  for (const double v : payoff) negated.push_back(-v);
  problem.lp.c = vec(negated);
  problem.lp.a_ub = mat(cost, 1, n);
  problem.lp.b_ub = vec({budget});
  problem.lp.lower = vec(std::vector<double>(static_cast<std::size_t>(n), 0.0));
  problem.lp.upper = vec(std::vector<double>(static_cast<std::size_t>(n), 1.0));
  problem.integer = vec(std::vector<double>(static_cast<std::size_t>(n), 1.0));  // every variable is an integer

  const auto r = milp::solve(problem);
  const auto x = r.x.host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "status " << (r.status == milp::Status::Optimal ? "Optimal" : "other") << ", payoff " << -r.objective.item<double>() << ", " << r.nodes << " nodes\nchosen:";
  double spent = 0.0;
  for (int j = 0; j < n; ++j)
    if (x[static_cast<std::size_t>(j)] > 0.5) { std::cout << " " << j; spent += cost[static_cast<std::size_t>(j)]; }
  std::cout << "   (cost " << spent << " of " << budget << ")\n";

  double best = 0.0;
  for (int mask = 0; mask < (1 << n); ++mask) {
    double c = 0.0, v = 0.0;
    for (int j = 0; j < n; ++j) if (mask & (1 << j)) { c += cost[static_cast<std::size_t>(j)]; v += payoff[static_cast<std::size_t>(j)]; }
    if (c <= budget && v > best) best = v;
  }
  std::cout << "brute force over 1024 subsets: payoff " << best << "\n";

  const auto relaxation = lp::solve(problem.lp);
  std::cout << "the linear relaxation (fractions allowed): payoff " << -relaxation.objective.item<double>() << "\n";
  return 0;
}
```

Output

```text
status Optimal, payoff 79.0000, 77 nodes
chosen: 1 2 3 6   (cost 40.0000 of 40.0000)
brute force over 1024 subsets: payoff 79.0000
the linear relaxation (fractions allowed): payoff 80.7692
```
The search proves the optimum in 77 nodes: take projects 1, 2, 3 and 6, for a payoff of 79 at a cost of exactly 40. The program checks it against a brute-force search of all 1024 subsets, which finds 79 too. Brute force is only possible for a handful of items, and it is the right way to test a formulation before trusting it on a large instance.

The last line is the relaxation: with fractions allowed, the payoff would be 80.7692, a little more than any whole selection can reach, because the relaxation can take a fraction of one project to use the budget exactly. The relaxation is always at least as good as the integer optimum, and the difference is the *integrality gap* the search has to close.

## Program 2: Integers that are not 0 or 1, and when the relaxation is enough

Integer variables can be any whole number in their bounds. The first program maximizes `5x + 4y + 3z` subject to three resource rows, with `x`, `y`, `z` integers up to 10. The second maximizes `x + y` subject to `x + y <= 3.5` with both integer.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto run = [](const char* label, milp::Problem p) {
    const auto relaxed = lp::solve(p.lp);
    const auto r = milp::solve(p);
    const auto x = r.x.host<double>();
    std::cout << std::left << std::setw(32) << label << "relaxation " << std::setw(9) << -relaxed.objective.item<double>() << "integer optimum " << std::setw(9)
              << -r.objective.item<double>() << "nodes " << std::setw(4) << r.nodes << "x =";
    for (const double v : x) std::cout << " " << v;
    std::cout << "\n";
  };

  {
    milp::Problem p;
    p.lp.c = vec({-5, -4, -3});
    p.lp.a_ub = mat({2, 3, 1, 4, 1, 2, 3, 4, 2}, 3, 3);
    p.lp.b_ub = vec({5, 11, 8});
    p.lp.lower = vec({0, 0, 0});
    p.lp.upper = vec({10, 10, 10});
    p.integer = vec({1, 1, 1});
    run("a small production plan", p);
  }
  {
    milp::Problem p;
    p.lp.c = vec({-1, -1});
    p.lp.a_ub = mat({1, 1}, 1, 2);
    p.lp.b_ub = vec({3.5});
    p.lp.lower = vec({0, 0});
    p.lp.upper = vec({10, 10});
    p.integer = vec({1, 1});
    run("x + y <= 3.5, both integer", p);
  }
  return 0;
}
```

Output

```text
a small production plan         relaxation 13.0000  integer optimum 13.0000  nodes 1   x = 2.0000 0.0000 1.0000
x + y <= 3.5, both integer      relaxation 3.5000   integer optimum 3.0000   nodes 17  x = 2.0000 1.0000
```
For the production plan the relaxation's answer is already whole, `(2, 0, 1)` with value 13, so the search finishes at the root: 1 node. This happens often enough to be worth knowing: solve the relaxation first, and if it is integral you are done. For `x + y <= 3.5` the relaxation reaches 3.5 and no integer point does; the search needs 17 nodes and finds 3, at `(2, 1)`.

## Program 3: How a search ends

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
    std::cout << "2 x = 1, x integer:   " << milp::to_string(r.status) << "\n\n";
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
    std::cout << std::left << std::setw(16) << limit << std::setw(12) << milp::to_string(r.status) << std::setw(8) << r.nodes << std::setw(14) << -r.objective.item<double>()
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

## Decisions and pitfalls

- **The relaxation is always at least as good as the integer optimum.** The gap between them is the room for improvement and the reason the search is needed.
- **Solve the relaxation first.** If it is integral you are done, with a one-node search.
- **Brute force is the right check for a handful of items.** Use it to verify a small model; it does not scale.
- **A node limit gives the best solution so far and a bound.** When the gap is small enough for your purpose, stop; otherwise raise the limit.
- **`Infeasible` can hide a feasible relaxation.** `2x = 1` with `x` an integer has a relaxation at 0.5 and no whole solution.

Where next: [Chapter 2](../milp_02_larger_integer_models.md) solves larger models with the sparse branch-and-cut solver.
