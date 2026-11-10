<!-- generated from septima_exit_flag_name.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::septima_exit_flag_name`

The name of a `SeptimaExitFlag` as text.

## Signature

```cpp
std::string_view optim::septima_exit_flag_name(optim::SeptimaExitFlag value);
```

## What It Does

Returns the enumerator name such as `Converged` or `NoProgress`; `Unknown` for an invalid value.

## Parameters

`value`
: The flag.

## Returns

A static name.

## Practical Examples

### Example 1

All six.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const optim::SeptimaExitFlag f : {optim::SeptimaExitFlag::Converged, optim::SeptimaExitFlag::MaxIterations, optim::SeptimaExitFlag::Stopped, optim::SeptimaExitFlag::NoProgress, optim::SeptimaExitFlag::NonFiniteEvaluation, optim::SeptimaExitFlag::InvalidProblem})
    out.writeln(static_cast<int>(f), "->", optim::septima_exit_flag_name(f));
}
```

Output

```text
1 -> Converged
0 -> MaxIterations
-1 -> Stopped
-2 -> NoProgress
-3 -> NonFiniteEvaluation
-4 -> InvalidProblem
```

### Example 2

Naming the flag of a solve.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  out.writeln("exit =", optim::septima_exit_flag_name(optim::septima(objective, pluss::series::make({0.0, 0.0}), options).exit_flag));
}
```

Output

```text
exit = Converged
```

### Example 3

A run with a tight budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  optim::SeptimaSolveOptions options;
  options.max_iterations = 3;
  out.writeln("exit =", optim::septima_exit_flag_name(optim::septima(objective, pluss::series::make({0.0, 0.0}), options).exit_flag));
}
```

Output

```text
exit = MaxIterations
```

## See Also

- [`SeptimaExitFlag`](SeptimaExitFlag.md): the enumeration

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="septima.html">
    <span class="label">Previous</span>
    <span class="title">septima</span>
  </a>
</div>
<!-- apiref:nav:end -->
