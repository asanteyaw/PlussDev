<!-- generated from the_step_s_linear_system.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The step's linear system

[Back to Curvature, Linear Algebra and Scaling](../optima_06_curvature_linear_algebra_scaling.md)


Each step ends in a linear system. A smoothing fit with 100 variables, 198 linear inequalities, one equality and bounds is solved with three solvers in both quadratic-model algorithms.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
using Block = optim::ParameterPack<pluss::Series>::Block;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const int n = 100;
  std::vector<double> target(n), start(n);
  double total = 0.0;
  for (int i = 0; i < n; ++i) {
    target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
    start[i] = (i % 2 == 0) ? 0.9 : 0.1;
    total += target[i];
  }
  std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0), cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
  for (int i = 0; i + 1 < n; ++i) {
    diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
    diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
  }
  const auto goal = optim::trace(vec(target), false);
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - goal)); };
  const auto constraints =
      optim::constraints()
          .ln_ineq(pluss::make_series(diff, {2 * (n - 1), n}).to(f64), vec(cap))
          .ln_eq(pluss::make_series(std::vector<double>(n, 1.0), {1, n}).to(f64), vec({total}))
          .lbs(vec(std::vector<double>(n, 0.0)))
          .ubs(vec(std::vector<double>(n, 1.0)));

  struct Row { const char* label; optim::OptimaLinearSystemSolver solver; optim::OptimaLinearSystemStructure structure; };
  const Row rows[] = {
      {"Cholesky", optim::OptimaLinearSystemSolver::Cholesky, optim::OptimaLinearSystemStructure::Dense},
      {"PivotedDirect", optim::OptimaLinearSystemSolver::PivotedDirect, optim::OptimaLinearSystemStructure::Dense},
      {"ConjugateGradient", optim::OptimaLinearSystemSolver::ConjugateGradient, optim::OptimaLinearSystemStructure::Sparse}};
  std::cout << std::left << std::setw(18) << "solver" << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7)
            << "iters" << std::setw(11) << "objective" << "krylov\n";
  for (const auto& row : rows) {
    for (const auto alg : {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::InternalProtocol}) {
      optim::OptimaOptions o;
      o.algorithm = alg;
      o.sequential_qp.linear_system_solver = row.solver;
      o.sequential_qp.system_structure = row.structure;
      o.internal_protocol.linear_system_solver = row.solver;
      o.internal_protocol.system_structure = row.structure;
      const auto r = optim::optima(objective, vec(start), o, constraints);
      std::cout << std::left << std::setw(18) << row.label << std::setw(18)
                << (alg == optim::OptimaAlgorithm::SequentialQP ? "SequentialQP" : "InternalProtocol") << std::setw(6)
                << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::fixed << std::setprecision(5)
                << std::setw(11) << r.evaluation.value.template item<double>() << std::setw(9) << r.output.krylov_iterations
                << "\n";
    }
  }
  return 0;
}
```

Output

```text
solver            algorithm         flag  iters  objective  krylov
Cholesky          SequentialQP      1     9      0.17534    0        
Cholesky          InternalProtocol  1     15     0.17535    0        
PivotedDirect     SequentialQP      1     9      0.17534    0        
PivotedDirect     InternalProtocol  1     15     0.17535    0        
ConjugateGradient SequentialQP      1     9      0.17534    11453    
ConjugateGradient InternalProtocol  1     15     0.17535    202      
```
All six runs reach the same objective, 0.17534 or 0.17535: sequential QP in 9 iterations and the interior method in 15. Conjugate gradients take 11453 Krylov iterations in sequential QP and 202 in the interior method; the factorizations take none because they form and factor the system once per step. A dense factorization of a 100-variable system is small work. Conjugate gradients exist for the problem sizes where the system no longer fits, `iterative_dimension_threshold` (3000 by default) is where `Auto` switches to them, and every algorithm falls back to a direct solve if conjugate gradients fail to reach their tolerance, so the choice can cost time but cannot cost the answer. The tolerance used inside the iteration is capped at 1e-8 whatever `krylov_tolerance` says, because a primal-dual step has to be accurate to the iteration's own precision.
