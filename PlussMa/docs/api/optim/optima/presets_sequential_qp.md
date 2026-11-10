<!-- generated from presets_sequential_qp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::presets::sequential_qp`

Options for sequential quadratic programming with a tight feasibility tolerance.

## Signature

```cpp
optim::OptimaOptions optim::presets::sequential_qp();
```

## What It Does

Returns an `OptimaOptions` with `algorithm = SequentialQP`, 100 iterations, a constraint tolerance of `1e-8` (tighter than the default `1e-6`), an optimality tolerance of `1e-6`, and `InitialPoint` scaling. Use it when feasibility must hold to high accuracy.

## Returns

An `OptimaOptions` ready to pass to `optima`.

## Practical Examples

### Example 1

Using the preset. It returns an ordinary `OptimaOptions`; the algorithm is `SequentialQP`.

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

  const optim::OptimaOptions options = optim::presets::sequential_qp();
  out.writeln("algorithm =", optim::optima_algorithm_name(options.algorithm));
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("x =", result.params.at("x").items<double>());
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
algorithm = SequentialQP
x = [0.447214, 0.894427]
exit = Converged , iterations = 4 , evaluations = 8
```

### Example 2

Adjusting a preset. A preset is a starting point: change any field afterwards.

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

  optim::OptimaOptions options = optim::presets::sequential_qp();
  options.sequential_qp.max_iterations = 3;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
exit = MaxIterations , iterations = 3 , evaluations = 7
```

### Example 3

What the preset changes: its values next to the defaults of the same algorithm.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const optim::OptimaOptions preset = optim::presets::sequential_qp();
  optim::OptimaOptions plain;
  plain.algorithm = preset.algorithm;
  std::cout << "constraint_tolerance = preset " << preset.sequential_qp.constraint_tolerance << ", default " << plain.sequential_qp.constraint_tolerance << "\n";
}
```

Output

```text
constraint_tolerance = preset 1e-08, default 1e-06
```

## See Also

- [`OptimaOptions`](OptimaOptions.md): the returned type
- [`optima`](optima.md): consumes it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="presets_robust_internal_protocol.html">
    <span class="label">Previous</span>
    <span class="title">robust_internal_protocol</span>
  </a>
  <a class="next-link" href="print_summary.html">
    <span class="label">Next</span>
    <span class="title">print_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
