<!-- generated from optima_problem_scaling_name.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::optima_problem_scaling_name`

The name of an `OptimaProblemScaling` as text.

## Signature

```cpp
std::string_view optim::optima_problem_scaling_name(optim::OptimaProblemScaling value);
```

## What It Does

Returns `None`, `InitialPoint` or `ObjectiveAndConstraints`; `Unknown` for an invalid value.

## Parameters

`value`
: The mode.

## Returns

A static name.

## Practical Examples

### Example 1

All three.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const optim::OptimaProblemScaling v : {optim::OptimaProblemScaling::None, optim::OptimaProblemScaling::InitialPoint, optim::OptimaProblemScaling::ObjectiveAndConstraints})
    out.writeln(static_cast<int>(v), "->", optim::optima_problem_scaling_name(v));
}
```

Output

```text
0 -> None
1 -> InitialPoint
2 -> ObjectiveAndConstraints
```

### Example 2

The default of an options block.

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
  out.writeln("sequential_qp :", optim::optima_problem_scaling_name(options.sequential_qp.problem_scaling));
  out.writeln("internal      :", optim::optima_problem_scaling_name(options.internal_protocol.problem_scaling));
}
```

Output

```text
sequential_qp : ObjectiveAndConstraints
internal      : ObjectiveAndConstraints
```

### Example 3

What a preset chooses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("sequential_qp preset :", optim::optima_problem_scaling_name(optim::presets::sequential_qp().sequential_qp.problem_scaling));
  out.writeln("internal preset      :", optim::optima_problem_scaling_name(optim::presets::internal_protocol().internal_protocol.problem_scaling));
}
```

Output

```text
sequential_qp preset : InitialPoint
internal preset      : InitialPoint
```

## See Also

- [`OptimaProblemScaling`](OptimaProblemScaling.md): the enumeration

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="optima_exit_flag_name.html">
    <span class="label">Previous</span>
    <span class="title">optima_exit_flag_name</span>
  </a>
  <a class="next-link" href="optima_progress_stage_name.html">
    <span class="label">Next</span>
    <span class="title">optima_progress_stage_name</span>
  </a>
</div>
<!-- apiref:nav:end -->
