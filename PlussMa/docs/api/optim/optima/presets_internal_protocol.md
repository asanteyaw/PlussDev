<!-- generated from presets_internal_protocol.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::presets::internal_protocol`

Options for the interior algorithm with the safeguards used by the broad test suite.

## Signature

```cpp
optim::OptimaOptions optim::presets::internal_protocol();
```

## What It Does

Returns an `OptimaOptions` with `algorithm = InternalProtocol` and a barrier schedule from `1e-2` down to `1e-8` (decay 0.2), tolerances `1e-8` on the constraints and `1e-6` on optimality, shortened line-search and restoration limits, a watchdog and second-order corrections on, and `InitialPoint` scaling.

## Returns

An `OptimaOptions` ready to pass to `optima`.

## Practical Examples

### Example 1

Using the preset. It returns an ordinary `OptimaOptions`; the algorithm is `InternalProtocol`.

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

  const optim::OptimaOptions options = optim::presets::internal_protocol();
  out.writeln("algorithm =", optim::optima_algorithm_name(options.algorithm));
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("x =", result.params.at("x").items<double>());
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
algorithm = InternalProtocol
x = [0.447214, 0.894427]
exit = Converged , iterations = 7 , evaluations = 13
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

  optim::OptimaOptions options = optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 3;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
exit = MaxIterations , iterations = 3 , evaluations = 9
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
  const optim::OptimaOptions preset = optim::presets::internal_protocol();
  optim::OptimaOptions plain;
  plain.algorithm = preset.algorithm;
  std::cout << "constraint_tolerance = preset " << preset.internal_protocol.constraint_tolerance << ", default " << plain.internal_protocol.constraint_tolerance << "\n";
  std::cout << "max_restoration_iterations = preset " << preset.internal_protocol.max_restoration_iterations << ", default " << plain.internal_protocol.max_restoration_iterations << "\n";
}
```

Output

```text
constraint_tolerance = preset 1e-08, default 1e-06
max_restoration_iterations = preset 4, default 8
```

## See Also

- [`OptimaOptions`](OptimaOptions.md): the returned type
- [`optima`](optima.md): consumes it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pack_vector.html">
    <span class="label">Previous</span>
    <span class="title">pack_vector</span>
  </a>
  <a class="next-link" href="presets_robust_internal_protocol.html">
    <span class="label">Next</span>
    <span class="title">robust_internal_protocol</span>
  </a>
</div>
<!-- apiref:nav:end -->
