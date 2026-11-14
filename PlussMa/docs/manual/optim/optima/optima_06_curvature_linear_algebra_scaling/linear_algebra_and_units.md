<!-- generated from linear_algebra_and_units.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The step's linear algebra, and units

[Back to Optima 6: Curvature, Linear Algebra and Scaling](../optima_06_curvature_linear_algebra_scaling.md)

Each step of the quadratic-model methods ends in a linear system, and this page covers the two things that decide how well that system behaves: how it is solved, and what units the problem is written in. The solver choices are a dense or sparse factorisation, which forms and factors the system once per step, and conjugate gradients, which solve it iteratively and pay off when the system is large or only available through products.

The second program is the harsher test. The problem is written in units a factor `K` away from the natural ones, and the table shows the exit flag and iteration count for every algorithm under every `problem_scaling` mode. Badly scaled problems are where steps fail and tolerances become meaningless, so the default (rescaling a copy of your problem by constants chosen from the starting point) is there for a reason.

## Program 5: The step's linear system

Each step ends in a linear system. A smoothing fit with 100 variables, 198 linear inequalities, one equality and bounds is solved with three solvers in both quadratic-model algorithms.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
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
  const auto goal = vec(target);
  auto objective = [&](const pluss::Series& x) { return (x - goal).square().sum(); };
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
ConjugateGradient SequentialQP      1     9      0.17534    11267    
ConjugateGradient InternalProtocol  1     15     0.17535    202      
```
All six runs reach the same objective, 0.17534 or 0.17535: sequential QP in 9 iterations and the interior method in 15. Conjugate gradients take 11267 Krylov iterations in sequential QP and 202 in the interior method; the factorizations take none because they form and factor the system once per step. A dense factorization of a 100-variable system is small work. Conjugate gradients exist for the problem sizes where the system no longer fits, `iterative_dimension_threshold` (3000 by default) is where `Auto` switches to them, and every algorithm falls back to a direct solve if conjugate gradients fail to reach their tolerance, so the choice can cost time but cannot cost the answer. The tolerance used inside the iteration is capped at 1e-8 whatever `krylov_tolerance` says, because a primal-dual step has to be accurate to the iteration's own precision.

## Program 6: Units, measured

The model has an objective and constraints written in units a factor `K` away from the natural ones. The table is exit flag and iteration count for every algorithm under every `problem_scaling` mode.

```cpp
#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using Block = optim::ParameterPack<pluss::Series>::Block;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  struct Alg { const char* label; optim::OptimaAlgorithm alg; };
  const Alg algs[] = {{"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  struct Mode { const char* label; optim::OptimaProblemScaling mode; };
  const Mode modes[] = {{"None", optim::OptimaProblemScaling::None},
                        {"InitialPoint", optim::OptimaProblemScaling::InitialPoint},
                        {"ObjectiveAndConstraints", optim::OptimaProblemScaling::ObjectiveAndConstraints}};
  std::cout << "cell = exit flag / iterations   (1 = converged)\n";
  std::cout << std::left << std::setw(8) << "K" << std::setw(18) << "algorithm";
  for (const auto& m : modes) std::cout << std::setw(26) << m.label;
  std::cout << "\n";
  for (double K : {1e3, 1e6, 1e9}) {
    for (const auto& a : algs) {
      std::ostringstream k;
      k << K;
      std::cout << std::left << std::setw(8) << k.str() << std::setw(18) << a.label;
      for (const auto& m : modes) {
        auto f = [K](const Pack& p) {
          return K * (p.at("a") - 1.0).square() + (1.0 / K) * (p.at("b") - 1500.0).square() + K * 0.01 * p.at("a");
        };
        const auto cons =
            optim::constraints()
                .nl_eq([K](const Pack& p) { return K * (p.at("a").square() + (p.at("b") * 0.001).square() - 3.0); })
                .nl_ineq([K](const Pack& p) { return (p.at("b") - 1400.0) / K; });
        optim::OptimaOptions o;
        o.algorithm = a.alg;
        o.sequential_qp.problem_scaling = m.mode;
        o.working_set.problem_scaling = m.mode;
        o.internal_protocol.problem_scaling = m.mode;
        o.sequential_qp.max_iterations = 300;
        o.working_set.max_iterations = 300;
        o.internal_protocol.max_iterations = 300;
        const auto r = optim::optima(f, optim::pack_scalars({{"a", scalar(0.3)}, {"b", scalar(900.0)}}), o, cons);
        std::cout << std::setw(26) << (std::to_string(static_cast<int>(r.exit_flag)) + " / " + std::to_string(r.iteration));
      }
      std::cout << "\n";
    }
  }
  return 0;
}
```

Output

```text
cell = exit flag / iterations   (1 = converged)
K       algorithm         None                      InitialPoint              ObjectiveAndConstraints   
1000    SequentialQP      1 / 10                    1 / 10                    1 / 16                    
1000    WorkingSet        1 / 43                    1 / 10                    1 / 23                    
1000    InternalProtocol  1 / 17                    1 / 17                    1 / 29                    
1e+06   SequentialQP      1 / 9                     1 / 9                     1 / 17                    
1e+06   WorkingSet        1 / 61                    1 / 60                    1 / 24                    
1e+06   InternalProtocol  -6 / 15                   -6 / 15                   1 / 49                    
1e+09   SequentialQP      -6 / 33                   -6 / 33                   1 / 19                    
1e+09   WorkingSet        1 / 11                    1 / 10                    1 / 35                    
1e+09   InternalProtocol  -6 / 20                   -6 / 20                   1 / 48                    
```
With `None`, the problem as written, sequential QP converges at `K = 1000` and `1e6` but fails at `1e9` (flag `-6`), and the interior method fails at both `1e6` and `1e9`; only the working-set method survives all three, in 43, 61 and 11 iterations. `InitialPoint` changes nothing for sequential QP and the interior method and moves the working-set method by at most a few iterations. `ObjectiveAndConstraints`, the default, is the only column in which every cell converges: sequential QP in 16, 17 and 19 iterations, the working-set method in 23, 24 and 35, and the interior method in 29, 49 and 48. The price is visible where the units were already fine: at `K = 1000` sequential QP takes 16 iterations instead of 10. The automatic rescaling is no substitute for a model whose numbers mean something, but it removes the failure that extreme units used to cause.

## Decisions and pitfalls

- **Factorise for small systems, iterate for large.** A dense factorisation of a 100-variable system is small work; conjugate gradients take thousands of Krylov iterations here and win only when forming the system is the cost.
- **All solvers reach the same answer.** The choice affects cost, not the result; compare `krylov_iterations` with the work of the factorisation to decide.
- **Leave `problem_scaling` on its default.** With `None` (the problem as written) sequential QP and the interior method fail at large `K` while the default mode succeeds; `InitialPoint` is not a safe alternative.
- **Write your problem in natural units if you can.** Scaling repairs a bad choice of units; it is not a licence to pick them carelessly, and tolerances are only meaningful when quantities are of order one.

Where next: [Chapter 7](../optima_07_derivatives.md) is about where the derivatives come from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="curvature_models.html">
    <span class="label">Previous</span>
    <span class="title">Curvature models</span>
  </a>
</div>
<!-- apiref:nav:end -->
