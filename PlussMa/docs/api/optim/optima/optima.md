<!-- generated from optima.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::optima`

Minimises an objective subject to bounds, linear rows and nonlinear constraints, and returns the answer with the multipliers and the reason it stopped.

## Signature

```cpp
optim::OptimaState optim::optima(const optim::ConstrainedProblem &problem, const optim::ParameterPack<pluss::Series> &initial, const optim::OptimaOptions &options = {});
optim::OptimaState optim::optima(const optim::ConstrainedProblem &problem, const pluss::Series &initial, const optim::OptimaOptions &options = {});
optim::OptimaState optim::optima(std::function<pluss::Series(const pluss::Series &)> objective, const pluss::Series &initial, const optim::OptimaOptions &options = {}, const optim::ObjectiveConstraints &constraints = {});
optim::OptimaState optim::optima(std::function<pluss::Series(const optim::ParameterPack<pluss::Series> &)> objective, const optim::ParameterPack<pluss::Series> &initial, const optim::OptimaOptions &options = {}, const optim::ObjectiveConstraints &constraints = {});
optim::OptimaState optim::optima(std::function<std::pair<pluss::Series, optim::ParameterPack<pluss::Series>>(const optim::ParameterPack<pluss::Series> &)> objective_with_gradient, const optim::ParameterPack<pluss::Series> &initial, const optim::OptimaOptions &options = {}, const optim::ObjectiveConstraints &constraints = {});
optim::OptimaState optim::optima(std::function<std::pair<pluss::Series, pluss::Series>(const pluss::Series &)> objective_with_gradient, const pluss::Series &initial, const optim::OptimaOptions &options = {}, const optim::ObjectiveConstraints &constraints = {});
```

## What It Does

`optima` finds a local minimiser of a smooth objective `f(x)` subject to any mix of five kinds of constraint: bounds `lower ≤ x ≤ upper`, linear equalities `Aeq x = beq`, linear inequalities `Aineq x ≤ bineq`, nonlinear equalities `h(x) = 0` and nonlinear inequalities `g(x) ≤ 0`. The objective may be written in two ways. A plain function of `Series` is differentiated automatically, which is the usual choice. A function that returns the value together with its gradient skips the differentiation, which is faster when the gradient is known in closed form.

The unknowns are a *parameter pack*: a list of named blocks, each a vector or a scalar, so a model with `mu` and `sigma` is written with those names rather than with positions in one long vector. The forms that take a plain `Series` use a single block called `x`. Constraints are bundled with [`optim::constraints()`](constraints.md) and passed as the last argument, or the whole problem is built once with [`optim::problem()`](problem.md) and passed as a value.

The algorithm is chosen in [`OptimaOptions`](OptimaOptions.md): sequential quadratic programming by default, plus a working-set variant, an interior (barrier) method and a penalty method. Whatever the algorithm, the answer is an [`OptimaState`](OptimaState.md): the point, its evaluation, the Lagrange multipliers, an exit flag, and an output record of how the run went. A run that does not converge returns normally with a flag that says why; check `converged` or `exit_flag` before trusting the point. Infinite bounds mean no bound.

## Parameters

`const ConstrainedProblem &problem`
: The objective and all constraints as one value, from `optim::problem()`.

`objective`
: A function of the parameters returning a scalar `Series`, or returning `(value, gradient)`.

`initial`
: The starting point: a `Series` (block `x`) or a `ParameterPack` of named blocks.

`const OptimaOptions &options`
: The algorithm and its settings.

`const ObjectiveConstraints &constraints`
: Bounds, linear rows and nonlinear constraints from `optim::constraints()`.

## Returns

An `OptimaState`.

## Errors

- `optim::optima: ...` when the problem is inconsistent: an undefined start, bounds named after a block that does not exist, a non-scalar objective, or a constraint row with the wrong number of columns.

## Practical Examples

### Example 1

A constrained problem in the shortest form. The unconstrained minimum of `|x − (1,2)|²` is outside the unit disc `|x|² ≤ 1`, so the answer is the point of the disc closest to it, on the circle, `(1,2)/√5`. The multiplier of the active constraint is positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);

  out.writeln("x =", result.params.at("x").items<double>());
  out.writeln("objective =", result.evaluation.value.item<double>());
  out.writeln("exit      =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration);
  out.writeln("multiplier of the disc =", result.multipliers.nonlinear_inequalities.items<double>());
}
```

Output

```text
x = [0.447214, 0.894427]
objective = 1.527864
exit      = Converged , iterations = 4
multiplier of the disc = [1.236068]
```

### Example 2

Named blocks. The unknowns `a` and `b` are scalars; the objective receives the whole pack and reads each block by name, and the answer is read back by the same names. Minimise `(a−1)² + 2(b+0.5)²` with `a ≥ 0.2` and `b ≥ 0.2`: `a` is free at 1, `b` sits on its bound.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Pack start = optim::pack_scalars({{"a", pluss::make_series(0.5)}, {"b", pluss::make_series(0.5)}});
  const Pack lower = optim::pack_scalars({{"a", pluss::make_series(0.2)}, {"b", pluss::make_series(0.2)}});
  const auto objective = [](const Pack &p) {
    return (p.at("a") - 1.0).square() + 2.0 * (p.at("b") + 0.5).square();
  };
  const optim::OptimaState result = optim::optima(objective, start, {}, optim::constraints().lbs(lower));

  out.writeln("a =", result.params.at("a").items<double>(), " b =", result.params.at("b").items<double>());
  out.writeln("converged =", result.converged);
}
```

Output

```text
a = [1.000000]  b = [0.200000]
converged = true
```

### Example 3

A problem value. The same objective and constraint are built once with `optim::problem()` and passed as a `ConstrainedProblem`; a problem can be solved again from other starting points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series target = pluss::series::make({1.0, 2.0});

  const optim::ConstrainedProblem problem =
      optim::problem()
          .objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); })
          .nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });
  for (const double s : {0.0, -0.5}) {
    const optim::OptimaState result = optim::optima(problem, pluss::series::make({s, s}));
    out.writeln("start", s, "-> x =", result.params.at("x").items<double>());
  }
}
```

Output

```text
start 0.000000 -> x = [0.447214, 0.894427]
start -0.500000 -> x = [0.447214, 0.894427]
```

### Example 4

An analytic gradient. The objective returns the pair `(value, gradient)`; no differentiation is done. Minimise `Σ (xᵢ − cᵢ)²` whose gradient is `2(x − c)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series c = pluss::series::make({1.0, -2.0, 3.0});
  const optim::OptimaState result = optim::optima(
      [&](const Pack &p) {
        const pluss::Series d = p.at("x") - c;
        return std::pair{d.square().sum(), optim::pack_series(2.0 * d)};
      },
      optim::pack_series(pluss::series::make({0.0, 0.0, 0.0})));

  out.writeln("x =", result.params.at("x").items<double>());
  out.writeln("function evaluations =", result.function_evaluations);
}
```

Output

```text
x = [1.000000, -2.000000, 3.000000]
function evaluations = 3
```

### Example 5

Choosing an algorithm. The same problem solved with each of the four algorithms by setting `OptimaOptions::algorithm`; they agree on the answer.

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

  for (const optim::OptimaAlgorithm algorithm : {optim::OptimaAlgorithm::MeritMethod, optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::InternalProtocol}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
    out.writeln(optim::optima_algorithm_name(algorithm), ": x =", result.params.at("x").items<double>(), ", exit =", optim::optima_exit_flag_name(result.exit_flag));
  }
}
```

Output

```text
MeritMethod : x = [0.4472, 0.8944] , exit = Converged
SequentialQP : x = [0.4472, 0.8944] , exit = Converged
WorkingSet : x = [0.4472, 0.8944] , exit = Converged
InternalProtocol : x = [0.4472, 0.8944] , exit = Converged
```

### Example 6

Two kinds of failure. A malformed problem (a lower bound above its upper bound) throws before any work is done; a run that merely does not finish returns a state whose flag says why. Here the iteration limit is set to 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto first_line = [](const std::exception &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };
  const auto squares = [](const pluss::Series &x) { return x.square().sum(); };
  try {
    (void)optim::optima(squares, pluss::series::make({1.5}), {},
                        optim::constraints().lbs(pluss::series::make({2.0})).ubs(pluss::series::make({1.0})));
  } catch (const std::exception &e) {
    out.writeln("error =", first_line(e));
  }
  optim::OptimaOptions short_run;
  short_run.sequential_qp.max_iterations = 1;
  const optim::OptimaState result = optim::optima([](const pluss::Series &x) { return (x - 3.0).square().sum() + 0.1 * x.pow(4).sum(); }, pluss::series::make({10.0}), short_run);
  out.writeln("converged =", result.converged, ", exit =", optim::optima_exit_flag_name(result.exit_flag));
}
```

Output

```text
error = optim::evaluate_problem: lower bound exceeds upper bound
converged = false , exit = MaxIterations
```

## See Also

- [`OptimaState`](OptimaState.md): the result
- [`OptimaOptions`](OptimaOptions.md): algorithm and tolerances
- [`problem`](problem.md): building a problem value
- [`constraints`](constraints.md): bundling constraints
- [`OptimaExitFlag`](OptimaExitFlag.md): why a run stopped

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="evaluate_problem.html">
    <span class="label">Previous</span>
    <span class="title">evaluate_problem</span>
  </a>
  <a class="next-link" href="optima_algorithm_name.html">
    <span class="label">Next</span>
    <span class="title">optima_algorithm_name</span>
  </a>
</div>
<!-- apiref:nav:end -->
