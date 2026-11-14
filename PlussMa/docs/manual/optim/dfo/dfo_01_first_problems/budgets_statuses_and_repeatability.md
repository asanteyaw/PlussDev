<!-- generated from budgets_statuses_and_repeatability.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Budgets, the five statuses and repeatability

[Back to Derivative-Free Optimization 1: First Problems](../dfo_01_first_problems.md)

Every derivative-free solve ends in one of five states, and reading the state correctly is most of using these methods well. `Converged` means the method's own stopping test was met. `ReachedEvaluationBudget` and `ReachedIterationBudget` mean the method was still improving when the budget ran out: the point is the best seen, usable, but not settled, and the remedy is to raise the budget and continue from the returned `minimizer`. `ReachedTimeBudget` means a time limit set in the method's own configuration ended the run, and `InfeasibleStationary` means that with constraints no feasible point was found.

Budgets are checked *between* steps, so a method can overshoot a budget by the evaluations of its last step. Methods that use random numbers read them from `rng_seed`: the same seed gives the same everything, a different seed a different path. On a function with many local minima a different seed ends in a different minimum, so running several seeds and keeping the best is a legitimate strategy.

## Program 4: Budgets and the five statuses

Every solve ends in one state. The budgets are checked between steps, so a method can overshoot a budget by the evaluations of its last step (60 allowed, 61 used below).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  std::vector<std::pair<const char*, dfo::DfoOptions>> cases;
  dfo::DfoOptions generous;
  cases.emplace_back("default budgets", generous);

  dfo::DfoOptions few_evals;
  few_evals.max_evaluations = 60;
  cases.emplace_back("max_evaluations = 60", few_evals);

  dfo::DfoOptions few_iters;
  few_iters.max_iterations = 25;
  cases.emplace_back("max_iterations = 25", few_iters);

  dfo::DfoOptions loose;
  loose.relative_improvement_tolerance = 1e-3;
  cases.emplace_back("tolerance 1e-3", loose);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, o] : cases) {
    o.solver = dfo::DfoSolverKind::NelderMead;
    const auto r = dfo::solve_dfo(rosenbrock, vec({-1.2, 1.0}), o);
    out.writeln(label, dfo::to_string(r.status), r.minimum, "evals", r.diagnostics.evaluations, "iterations", r.diagnostics.iterations);
  }
}
```

Output

```text
default budgets Converged 8.18e-10 evals 159 iterations 85
max_evaluations = 60 ReachedEvaluationBudget 6.69e-01 evals 61 iterations 32
max_iterations = 25 ReachedIterationBudget 1.32e+00 evals 49 iterations 25
tolerance 1e-3 Converged 1.00e-07 evals 143 iterations 77
```
How to read the endings:

- `Converged`: the method's stopping test was met.
- `ReachedEvaluationBudget` / `ReachedIterationBudget`: it was still improving when the budget ran out. The point is the best seen, usable, but not settled. Raise the budget and run again, starting from the returned `minimizer`.
- `ReachedTimeBudget`: a time limit set in the method's own configuration ended the run.
- `InfeasibleStationary`: with constraints, no feasible point was found (Chapter 3).

A looser `relative_improvement_tolerance` stops earlier with a coarser answer; the last row shows the trade.

## Program 5: Seeds and repeatability

Methods that use random numbers read them from `rng_seed`. Same seed, same everything; different seed, different path.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }

int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  std::cout << std::left << std::setw(8) << "seed" << std::setw(26) << "status" << std::setw(14) << "minimum" << "evaluations\n";
  for (std::uint64_t seed : {1ULL, 2ULL, 1ULL, 5489ULL, 5489ULL}) {
    dfo::DfoProblem p;
    p.initial = filled(3, 3.0);
    p.lower_bounds = filled(3, -5.12);
    p.upper_bounds = filled(3, 5.12);
    p.objective = rastrigin;
    dfo::DfoOptions o;
    o.solver = dfo::DfoSolverKind::DifferentialEvolution;
    o.max_evaluations = 4000;
    o.rng_seed = seed;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(8) << seed << std::setw(26) << dfo::to_string(r.status) << std::scientific
              << std::setprecision(3) << std::setw(14) << r.minimum << std::defaultfloat << r.diagnostics.evaluations << "\n";
  }
}
```

Output

```text
seed    status                    minimum       evaluations
1       ReachedEvaluationBudget   4.858e+00     4000
2       ReachedEvaluationBudget   4.955e+00     4000
1       ReachedEvaluationBudget   4.858e+00     4000
5489    ReachedEvaluationBudget   4.729e+00     4000
5489    ReachedEvaluationBudget   4.729e+00     4000
```
Rows 1 and 3 are identical, and so are rows 4 and 5. The Rastrigin function used here has many local minima, so different seeds end in different ones; with a fixed budget, running several seeds and keeping the best is a legitimate strategy for such problems.

## Decisions and pitfalls

- **A budget status is not a failure.** A population method that ends on `ReachedEvaluationBudget` with a value of 1e-12 has done well; it simply has no cheap stopping test.
- **Continue from the returned `minimizer`.** To go further after a budget status, start a new run at the returned point with a larger budget.
- **Set the seed for reproducibility, and vary it for robustness.** The same seed repeats a run exactly; several seeds explore several minima.
- **A looser `relative_improvement_tolerance` stops earlier with a coarser answer.** It is the knob that trades accuracy for evaluations.

Where next: [Chapter 2](../dfo_02_the_solvers.md) compares the seven methods.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="first_solves.html">
    <span class="label">Previous</span>
    <span class="title">First derivative-free solves</span>
  </a>
</div>
<!-- apiref:nav:end -->
