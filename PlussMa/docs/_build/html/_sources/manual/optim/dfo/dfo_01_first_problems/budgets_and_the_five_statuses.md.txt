<!-- generated from budgets_and_the_five_statuses.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Budgets and the five statuses

[Back to First Problems](../dfo_01_first_problems.md)


Every solve ends in one state. The budgets are checked between steps, so a method can overshoot a budget by the evaluations of its last step (60 allowed, 61 used below).

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

void run(const char* label, dfo::DfoOptions o) {
  o.solver = dfo::DfoSolverKind::NelderMead;
  const auto r = dfo::solve_dfo(rosenbrock, vec({-1.2, 1.0}), o);
  std::cout << std::left << std::setw(26) << label << std::setw(26) << dfo::to_string(r.status) << std::scientific
            << std::setprecision(2) << std::setw(11) << r.minimum << std::defaultfloat << "evals " << std::setw(6)
            << r.diagnostics.evaluations << "iters " << r.diagnostics.iterations << "\n";
}

int main() {
  dfo::DfoOptions generous;
  run("default budgets", generous);

  dfo::DfoOptions few_evals;
  few_evals.max_evaluations = 60;
  run("max_evaluations = 60", few_evals);

  dfo::DfoOptions few_iters;
  few_iters.max_iterations = 25;
  run("max_iterations = 25", few_iters);

  dfo::DfoOptions loose;
  loose.relative_improvement_tolerance = 1e-3;
  run("tolerance 1e-3", loose);
}
```

Output

```text
default budgets           Converged                 8.18e-10   evals 159   iters 85
max_evaluations = 60      ReachedEvaluationBudget   6.69e-01   evals 61    iters 32
max_iterations = 25       ReachedIterationBudget    1.32e+00   evals 49    iters 25
tolerance 1e-3            Converged                 1.00e-07   evals 143   iters 77
```
How to read the endings:

- `Converged`: the method's stopping test was met.
- `ReachedEvaluationBudget` / `ReachedIterationBudget`: it was still improving when the budget ran out. The point is the best seen, usable, but not settled. Raise the budget and run again, starting from the returned `minimizer`.
- `ReachedTimeBudget`: a time limit set in the method's own configuration ended the run.
- `InfeasibleStationary`: with constraints, no feasible point was found (Chapter 3).

A looser `relative_improvement_tolerance` stops earlier with a coarser answer; the last row shows the trade.
