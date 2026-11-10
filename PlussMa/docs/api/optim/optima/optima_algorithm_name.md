<!-- generated from optima_algorithm_name.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::optima_algorithm_name`

The name of an `OptimaAlgorithm` as text.

## Signature

```cpp
std::string_view optim::optima_algorithm_name(optim::OptimaAlgorithm value);
```

## What It Does

Returns `MeritMethod`, `SequentialQP`, `WorkingSet` or `InternalProtocol`; `Unknown` for an invalid value. The name is a view of a static string.

## Parameters

`value`
: The algorithm.

## Returns

A static name.

## Practical Examples

### Example 1

All four.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const optim::OptimaAlgorithm a : {optim::OptimaAlgorithm::MeritMethod, optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::InternalProtocol})
    out.writeln(static_cast<int>(a), "->", optim::optima_algorithm_name(a));
}
```

Output

```text
0 -> MeritMethod
1 -> SequentialQP
2 -> WorkingSet
3 -> InternalProtocol
```

### Example 2

In a report line.

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
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);
  out.writeln("ran", optim::optima_algorithm_name(result.algorithm), "in", result.iteration, "iterations");
}
```

Output

```text
ran SequentialQP in 4 iterations
```

### Example 3

Mixed with the default-constructed option.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::OptimaOptions options;
  out.writeln("default algorithm:", optim::optima_algorithm_name(options.algorithm));
}
```

Output

```text
default algorithm: SequentialQP
```

## See Also

- [`OptimaAlgorithm`](OptimaAlgorithm.md): the enumeration

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="optima.html">
    <span class="label">Previous</span>
    <span class="title">optima</span>
  </a>
  <a class="next-link" href="optima_exit_flag_name.html">
    <span class="label">Next</span>
    <span class="title">optima_exit_flag_name</span>
  </a>
</div>
<!-- apiref:nav:end -->
