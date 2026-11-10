<!-- generated from lsr1_exit_flag_name.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::lsr1_exit_flag_name`

The name of an `Lsr1ExitFlag` as text.

## Signature

```cpp
std::string_view optim::lsr1_exit_flag_name(optim::Lsr1ExitFlag value);
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
  for (const optim::Lsr1ExitFlag f : {optim::Lsr1ExitFlag::Converged, optim::Lsr1ExitFlag::MaxIterations, optim::Lsr1ExitFlag::Stopped, optim::Lsr1ExitFlag::NoProgress, optim::Lsr1ExitFlag::NonFiniteEvaluation, optim::Lsr1ExitFlag::InvalidProblem})
    out.writeln(static_cast<int>(f), "->", optim::lsr1_exit_flag_name(f));
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

  out.writeln("exit =", optim::lsr1_exit_flag_name(optim::l_sr1_trust_region(objective, pluss::series::make({0.0, 0.0})).exit_flag));
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
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (1.0 - a).square() + 100.0 * (b - a.square()).square();
  };

  optim::Lsr1SolveOptions options;
  options.max_iterations = 3;
  out.writeln("exit =", optim::lsr1_exit_flag_name(optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0}), options).exit_flag));
}
```

Output

```text
exit = MaxIterations
```

## See Also

- [`Lsr1ExitFlag`](Lsr1ExitFlag.md): the enumeration

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="l_sr1_trust_region.html">
    <span class="label">Previous</span>
    <span class="title">l_sr1_trust_region</span>
  </a>
  <a class="next-link" href="septima.html">
    <span class="label">Next</span>
    <span class="title">septima</span>
  </a>
</div>
<!-- apiref:nav:end -->
