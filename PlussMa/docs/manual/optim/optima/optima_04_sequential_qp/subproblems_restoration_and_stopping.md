<!-- generated from subproblems_restoration_and_stopping.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Subproblems, restoration and stopping

[Back to Optima 4: Sequential QP in Depth](../optima_04_sequential_qp.md)

The loop of the previous page can be stressed in three places, and this page shows each. **Restoration** is the last resort: when the line search fails twice in a row, SQP stops chasing the objective and concentrates on reducing the constraint violation alone. It is a safety net for a line search that has an adequate budget but meets a bad region, and the program shows that it cannot rescue a line search that has been starved of trial steps.

The **subproblem solver** is how each quadratic subproblem is solved: it reduces to a symmetric positive-definite system in the variables, so the usual choices apply (a direct factorisation or conjugate gradients, with several variants of each). For a handful of variables a factorisation is right and `Auto` picks it; conjugate gradients pay off when the number of variables passes `iterative_dimension_threshold`. Finally the **stopping tolerances** decide when the run is done: tightening them moves the answer and the cost, and the last program shows what the tightest setting runs into.

## Program 5: Restoration, the last resort

Restoration runs only after the line search has failed twice in a row, so the program starves the line search on purpose to reach it.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  const auto start = optim::pack_scalars({{"x", scalar(-2.0)}, {"y", scalar(-1.2)}});
  auto f = [](const Pack& p) { return 2.0 * (p.at("x").square() + p.at("y").square() - 1.0) - p.at("x"); };
  const auto cons = optim::constraints().nl_eq(
      [](const Pack& p) { return p.at("x").square() + p.at("y").square() - 1.0; });

  std::cout << std::left << std::setw(26) << "max_line_search_iterations" << std::setw(13) << "restoration" << std::setw(6)
            << "flag" << std::setw(7) << "iters" << std::setw(7) << "evals" << "message\n";
  for (std::int64_t budget : {1, 3, 20}) {
    for (bool on : {true, false}) {
      optim::OptimaOptions o;
      o.algorithm = optim::OptimaAlgorithm::SequentialQP;
      o.sequential_qp.max_line_search_iterations = budget;
      o.sequential_qp.max_second_order_corrections = 0;
      o.sequential_qp.enable_restoration = on;
      const auto r = optim::optima(f, start, o, cons);
      std::cout << std::left << std::setw(26) << budget << std::setw(13) << (on ? "enabled" : "disabled") << std::setw(6)
                << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
                << r.message.substr(0, 56) << "\n";
    }
  }
  return 0;
}
```

Output

```text
max_line_search_iterationsrestoration  flag  iters  evals  message
1                         enabled      -6    3      21     line search failed to produce an acceptable SQP step dia
1                         disabled     -6    2      6      line search failed to produce an acceptable SQP step dia
3                         enabled      1     9      18     optimality and feasibility tolerances satisfied
3                         disabled     1     9      18     optimality and feasibility tolerances satisfied
20                        enabled      1     12     22     optimality and feasibility tolerances satisfied
20                        disabled     1     12     22     optimality and feasibility tolerances satisfied
```
With a budget of 3 or more trial steps the two settings are identical, so restoration never ran. With a budget of 1, restoration is entered, spends 21 evaluations against 6, and still ends in failure. Restoration is a safety net for a line search that has an adequate budget but meets a bad region; it cannot repair a line search that has been starved. If you see the failure message, raise `max_line_search_iterations` first.

## Program 6: The subproblem solvers

Each subproblem is reduced to a symmetric positive-definite system in the variables, so the usual solver choices apply.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto objective = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  struct Row { const char* name; optim::OptimaLinearSystemSolver solver; optim::OptimaLinearSystemStructure structure; };
  const Row rows[] = {
      {"Auto", optim::OptimaLinearSystemSolver::Auto, optim::OptimaLinearSystemStructure::Auto},
      {"Cholesky", optim::OptimaLinearSystemSolver::Cholesky, optim::OptimaLinearSystemStructure::Dense},
      {"PivotedDirect", optim::OptimaLinearSystemSolver::PivotedDirect, optim::OptimaLinearSystemStructure::Dense},
      {"Inverse", optim::OptimaLinearSystemSolver::Inverse, optim::OptimaLinearSystemStructure::Dense},
      {"PseudoInverse", optim::OptimaLinearSystemSolver::PseudoInverse, optim::OptimaLinearSystemStructure::Dense},
      {"ConjugateGradient", optim::OptimaLinearSystemSolver::ConjugateGradient, optim::OptimaLinearSystemStructure::Sparse},
  };
  std::cout << std::left << std::setw(20) << "solver" << std::setw(7) << "iters" << std::setw(14) << "objective"
            << "krylov iterations\n";
  for (const auto& row : rows) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.linear_system_solver = row.solver;
    o.sequential_qp.system_structure = row.structure;
    const auto r = optim::optima(objective, four(1, 5, 5, 1), o, constraints());
    std::cout << std::left << std::setw(20) << row.name << std::setw(7) << r.iteration << std::fixed << std::setprecision(6)
              << std::setw(14) << r.evaluation.value.item<double>() << r.output.krylov_iterations << "\n";
  }
  return 0;
}
```

Output

```text
solver              iters  objective     krylov iterations
Auto                4      17.014017     0
Cholesky            4      17.014017     0
PivotedDirect       4      17.014017     0
Inverse             4      17.014017     0
PseudoInverse       4      17.014017     0
ConjugateGradient   4      17.014017     323
```
All six choices reach 17.014017 in 4 iterations. The conjugate-gradient row reports 323 Krylov iterations; every other row reports 0. For a problem with a handful of variables the factorization is the right choice and `Auto` picks it. Conjugate gradients pay off when the number of variables passes `iterative_dimension_threshold` (3000 by default), where forming and factoring the matrix costs more than a modest number of matrix-vector products. The solver keeps the interior iteration accurate by capping the inner relative tolerance at 1e-8, and if the reduced system stalls, whatever the solver, it re-solves the subproblem on the unreduced system with a pivoted factorization, so a solver choice cannot turn a solvable subproblem into a failure.

## Program 7: Stopping tolerances

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto objective = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  for (double tol : {1e-3, 1e-6, 1e-9}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.optimality_tolerance = tol;
    o.sequential_qp.constraint_tolerance = tol;
    const auto r = optim::optima(objective, four(1, 5, 5, 1), o, constraints());
    std::cout << std::scientific << std::setprecision(0) << "tolerances " << tol << ": " << std::fixed << std::setprecision(8)
              << r.evaluation.value.template item<double>() << ", " << r.iteration << " iterations, \""
              << r.message.substr(0, r.message.find('\n')) << "\"\n";
  }
  return 0;
}
```

Output

```text
tolerances 1e-03: 17.01371683, 3 iterations, "optimality and feasibility tolerances satisfied"
tolerances 1e-06: 17.01401725, 4 iterations, "optimality and feasibility tolerances satisfied"
tolerances 1e-09: 17.01401729, 5 iterations, "predicted objective improvement below tolerance at a feasible point"
```
Tightening the optimality and constraint tolerances together from 1e-3 to 1e-9 moves the objective from 17.01371683 to 17.01401729 and the iteration count from 3 to 5. The tightest run stops on "predicted objective improvement below tolerance at a feasible point" rather than on the optimality test: at 1e-9 the remaining improvement is below what the objective can resolve, which is the sign to stop asking for more precision. Ask for the precision your decision needs, not more: the three runs agree to four decimals.

## Decisions and pitfalls

- **Restoration is not a fix for a starved line search.** If you see the failure message after restoration, give the line search more trial steps before blaming the problem.
- **Leave the subproblem solver on `Auto` below a few thousand variables.** Choices that give the same path (here, all six reach the same value in 4 iterations) differ only in cost.
- **Tolerances trade accuracy for cost, with a floor.** Past a point the remaining improvement is below what the objective can resolve, and the run stops on a different test; do not chase digits the objective cannot supply.
- **Tighten the optimality and constraint tolerances together.** Tightening one alone leaves the other limiting the answer.
- **Check which stopping test fired.** The message tells you whether the run ended on optimality or on a predicted-improvement test.

Where next: [Chapter 5](../optima_05_working_set_and_interior.md) turns to the other two methods.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_sqp_loop.html">
    <span class="label">Previous</span>
    <span class="title">The sequential QP loop</span>
  </a>
</div>
<!-- apiref:nav:end -->
