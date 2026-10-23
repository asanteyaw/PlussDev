<!-- generated from the_subproblem_solvers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The subproblem solvers

[Back to Sequential QP in Depth](../optima_04_sequential_qp.md)


Each subproblem is reduced to a symmetric positive-definite system in the variables, so the usual solver choices apply.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

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
        return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) + optim::square(p.at("d")) - 40.0;
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
ConjugateGradient   4      17.014017     324
```
All six choices reach 17.014017 in 4 iterations. The conjugate-gradient row reports 324 Krylov iterations; every other row reports 0. For a problem with a handful of variables the factorization is the right choice and `Auto` picks it. Conjugate gradients pay off when the number of variables passes `iterative_dimension_threshold` (3000 by default), where forming and factoring the matrix costs more than a modest number of matrix-vector products. The solver keeps the interior iteration accurate by capping the inner relative tolerance at 1e-8, and if the reduced system stalls, whatever the solver, it re-solves the subproblem on the unreduced system with a pivoted factorization, so a solver choice cannot turn a solvable subproblem into a failure.
