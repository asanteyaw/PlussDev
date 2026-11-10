<!-- generated from Lsr1State.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::Lsr1State`

The memory of the L-SR1 method at the end of a run: stored pairs, the scalar gamma and the trust radius.

Held in `Lsr1Result::state`. `s_history` and `y_history` are the stored step and gradient-difference pairs, `prev_grad_flat` the previous gradient, `gamma` the scalar of the initial model (0 means not yet set), `radius` the trust-region radius at the end (0 means not yet set), and `step` the number of steps taken.

## Fields

### `s_history`

`std::vector<pluss::Series>`, default `empty`.

Stored steps, most recent last.

### `y_history`

`std::vector<pluss::Series>`, default `empty`.

Stored gradient differences.

### `prev_grad_flat`

`pluss::Series`, default `undefined`.

The previous gradient.

### `gamma`

`double`, default `0`.

Scalar of the initial model; 0 means unset.

### `radius`

`double`, default `0`.

Trust-region radius; 0 means unset.

### `step`

`std::int64_t`, default `0`.

Steps taken.

## Guidance

Read from `result.state`.

## Practical Examples

### Example 1

Stored pairs and the final radius.

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

  const optim::Lsr1Result result = optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln("stored pairs =", result.state.s_history.size(), ", steps =", result.state.step, ", gamma =", result.state.gamma, ", radius =", result.state.radius);
}
```

Output

```text
stored pairs = 2 , steps = 43 , gamma = 0.3596 , radius = 1.0000
```

### Example 2

The memory limit. With `memory_size = 2` no more than two pairs are kept.

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
  options.step.memory_size = 2;
  const optim::Lsr1Result result = optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
  out.writeln("stored pairs =", result.state.s_history.size());
}
```

Output

```text
stored pairs = 2
```

### Example 3

A fresh state is empty.

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
  const optim::Lsr1State state;
  out.writeln("pairs =", state.s_history.size(), ", gamma =", state.gamma, ", radius =", state.radius, ", step =", state.step);
}
```

Output

```text
pairs = 0 , gamma = 0.0000 , radius = 0.0000 , step = 0
```

## See Also

- [`Lsr1Result`](Lsr1Result.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Lsr1SolveOptions.html">
    <span class="label">Previous</span>
    <span class="title">Lsr1SolveOptions</span>
  </a>
  <a class="next-link" href="SeptimaConstraints.html">
    <span class="label">Next</span>
    <span class="title">SeptimaConstraints</span>
  </a>
</div>
<!-- apiref:nav:end -->
