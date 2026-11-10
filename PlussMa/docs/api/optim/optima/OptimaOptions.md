<!-- generated from OptimaOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::OptimaOptions`

Chooses the algorithm of `optima` and carries the settings of each algorithm.

`OptimaOptions` holds the choice `algorithm` and one settings block per algorithm: [`penalty`](PenaltyMethodOptions.md) for the penalty (merit) method, [`sequential_qp`](SequentialQPOptions.md), [`working_set`](WorkingSetOptions.md) and [`internal_protocol`](InternalProtocolOptions.md). Only the block of the chosen algorithm is read; the others are ignored. The default is sequential quadratic programming, which is the right choice for most smooth problems. The ready-made [presets](presets_sequential_qp.md) return an `OptimaOptions` already tuned.

`progress_hook` is a function called with an [`OptimaProgressSnapshot`](reports_iter.md) after every iteration; returning `true` stops the run (exit flag `Stopped`). A hook set on the top-level options is used by whichever algorithm runs; a hook set on one algorithm's own block takes precedence over it.

## Fields

### `algorithm`

`optim::OptimaAlgorithm`, default `SequentialQP`.

Which method solves the problem.

### `penalty`

`optim::PenaltyMethodOptions`, default `{}`.

Settings of the penalty (merit) method, used when `algorithm` is `MeritMethod`.

### `sequential_qp`

`optim::SequentialQPOptions`, default `{}`.

Settings of sequential quadratic programming, used when `algorithm` is `SequentialQP`.

### `working_set`

`optim::WorkingSetOptions`, default `{}`.

Settings of the working-set variant, used when `algorithm` is `WorkingSet`.

### `internal_protocol`

`optim::InternalProtocolOptions`, default `{}`.

Settings of the interior (barrier) method, used when `algorithm` is `InternalProtocol`.

### `progress_hook`

`std::function<bool(const optim::OptimaProgressSnapshot &)>`, default `empty`.

Called after each iteration with a snapshot; return `true` to stop. Overridden by the hook inside the chosen algorithm's own block.

## Guidance

Used as the third argument of `optima`.

## Practical Examples

### Example 1

The default: sequential quadratic programming.

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
  out.writeln("algorithm =", optim::optima_algorithm_name(options.algorithm));
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
algorithm = SequentialQP
x = [0.4472, 0.8944]
```

### Example 2

Choosing the interior method and tightening its tolerance.

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

  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.optimality_tolerance = 1e-9;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("x =", result.params.at("x").items<double>());
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
x = [0.447214, 0.894427]
exit = Converged , iterations = 8 , evaluations = 14
```

### Example 3

A progress hook that stops the run. It receives a snapshot per iteration and returns `true` to stop; here after the second iteration.

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
  options.progress_hook = [](const optim::OptimaProgressSnapshot &snapshot) { return snapshot.iteration >= 2; };
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration);
}
```

Output

```text
exit = Stopped , iterations = 2
```

### Example 4

One options value per algorithm, kept side by side; only the chosen block is read.

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
  options.sequential_qp.max_iterations = 50;
  options.internal_protocol.max_iterations = 200;
  options.algorithm = optim::OptimaAlgorithm::SequentialQP;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
exit = Converged , iterations = 4 , evaluations = 8
```

## See Also

- [`optima`](optima.md): consumes Options
- [`OptimaAlgorithm`](OptimaAlgorithm.md): the four choices

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OptimaLinearSystemStructure.html">
    <span class="label">Previous</span>
    <span class="title">OptimaLinearSystemStructure</span>
  </a>
  <a class="next-link" href="OptimaOutput.html">
    <span class="label">Next</span>
    <span class="title">OptimaOutput</span>
  </a>
</div>
<!-- apiref:nav:end -->
