<!-- generated from AdamState.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::AdamState`

The running state of Adam: the moment estimates and the step count.

`m` and `v` hold, per parameter block, the running averages of the gradient and of its square; `max_v` holds their running maximum for AMSGrad; `step` counts the steps taken and drives the bias correction. [`adam_state_like`](adam_state_like.md) builds a fresh zero state for a list of parameters. [`adam_step`](adam_step.md) and [`adam_step_inplace`](adam_step_inplace.md) update it; `septima` keeps its own in `SeptimaState::adam`.

## Fields

### `m`

`std::vector<pluss::Series>`, default `empty`.

Running average of the gradient per block.

### `v`

`std::vector<pluss::Series>`, default `empty`.

Running average of its square.

### `max_v`

`std::vector<pluss::Series>`, default `empty`.

Running maximum of `v` (AMSGrad).

### `step`

`std::int64_t`, default `0`.

Steps taken.

## Guidance

Passed to `adam_step`.

## Practical Examples

### Example 1

A fresh state is zero with the parameters' shapes.

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
  const optim::AdamState state = optim::adam_state_like({pluss::series::make({1.0, 2.0})});
  out.writeln("blocks =", state.m.size(), ", step =", state.step, ", m =", state.m[0].items<double>());
}
```

Output

```text
blocks = 1 , step = 0 , m = [0.0000, 0.0000]
```

### Example 2

The state advances with each step.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<Series> params{pluss::series::make({1.0, 2.0})};
  optim::AdamState state = optim::adam_state_like(params);
  for (int i = 0; i < 3; ++i) optim::adam_step_inplace(params, {2.0 * params[0]}, state, {.learning_rate = 0.1});
  out.writeln("step =", state.step, ", m =", state.m[0].items<double>());
}
```

Output

```text
step = 3 , m = [0.4841, 1.0260]
```

### Example 3

The state at the end of a `septima` run.

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
  options.max_iterations = 10;
  const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("Adam steps taken =", result.state.adam.step);
}
```

Output

```text
Adam steps taken = 10
```

## See Also

- [`adam_state_like`](adam_state_like.md): creates one
- [`adam_step`](adam_step.md): updates one

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="AdamOptions.html">
    <span class="label">Previous</span>
    <span class="title">AdamOptions</span>
  </a>
  <a class="next-link" href="LbfgsOptions.html">
    <span class="label">Next</span>
    <span class="title">LbfgsOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
