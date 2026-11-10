<!-- generated from OptimaLinearSystemSolver.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::OptimaLinearSystemSolver`

How the linear system inside each iteration is solved.

Each quadratic subproblem reduces to a symmetric positive-definite system. `Auto` (the default) uses Cholesky and switches to conjugate gradients when the number of unknowns reaches `iterative_dimension_threshold`. `Cholesky` is the fastest direct method for positive-definite systems. `PivotedDirect` is a pivoted LU, robust to near-singularity. `Inverse` and `PseudoInverse` form an explicit inverse or a pseudo-inverse; they are for diagnosing a singular system rather than for speed. `ConjugateGradient` is the iterative solve, with its tolerances in the options (`krylov_tolerance`, `max_krylov_iterations`), the right choice for very many unknowns.

## Fields

### `Auto`

`enumerator`, default `0`.

Cholesky, or conjugate gradients for many unknowns.

### `Cholesky`

`enumerator`, default `1`.

Direct factorisation.

### `PivotedDirect`

`enumerator`, default `2`.

Pivoted LU.

### `Inverse`

`enumerator`, default `3`.

Explicit inverse.

### `PseudoInverse`

`enumerator`, default `4`.

Pseudo-inverse.

### `ConjugateGradient`

`enumerator`, default `5`.

Iterative.

## Guidance

Set as `linear_system_solver` in the options of an algorithm.

## Practical Examples

### Example 1

Each solver on the same problem. They agree on the answer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  const std::pair<const char *, optim::OptimaLinearSystemSolver> solvers[] = {
      {"Auto", optim::OptimaLinearSystemSolver::Auto}, {"Cholesky", optim::OptimaLinearSystemSolver::Cholesky}, {"PivotedDirect", optim::OptimaLinearSystemSolver::PivotedDirect},
      {"PseudoInverse", optim::OptimaLinearSystemSolver::PseudoInverse}, {"ConjugateGradient", optim::OptimaLinearSystemSolver::ConjugateGradient}};
  for (const auto &[name, solver] : solvers) {
    optim::OptimaOptions options;
    options.sequential_qp.linear_system_solver = solver;
    const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
    out.writeln(name, ": x =", result.params.at("x").items<double>(), ", krylov iterations =", result.output.krylov_iterations);
  }
}
```

Output

```text
Auto : x = [0.4472, 0.8944] , krylov iterations = 0
Cholesky : x = [0.4472, 0.8944] , krylov iterations = 0
PivotedDirect : x = [0.4472, 0.8944] , krylov iterations = 0
PseudoInverse : x = [0.4472, 0.8944] , krylov iterations = 0
ConjugateGradient : x = [0.4472, 0.8944] , krylov iterations = 110
```

### Example 2

Conjugate gradients with their settings. The relative tolerance is capped at 1e-8 because the interior iteration needs accurate steps.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.sequential_qp.linear_system_solver = optim::OptimaLinearSystemSolver::ConjugateGradient;
  options.sequential_qp.krylov_tolerance = 1e-10;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("x =", result.params.at("x").items<double>());
  out.writeln("krylov iterations =", result.output.krylov_iterations);
}
```

Output

```text
x = [0.4472, 0.8944]
krylov iterations = 110
```

### Example 3

In the interior algorithm.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.linear_system_solver = optim::OptimaLinearSystemSolver::PivotedDirect;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
exit = Converged , iterations = 7 , evaluations = 13
```

## See Also

- [`OptimaLinearSystemStructure`](OptimaLinearSystemStructure.md): dense or sparse
- [`SequentialQPOptions`](SequentialQPOptions.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OptimaHessianApproximation.html">
    <span class="label">Previous</span>
    <span class="title">OptimaHessianApproximation</span>
  </a>
  <a class="next-link" href="OptimaLinearSystemStructure.html">
    <span class="label">Next</span>
    <span class="title">OptimaLinearSystemStructure</span>
  </a>
</div>
<!-- apiref:nav:end -->
