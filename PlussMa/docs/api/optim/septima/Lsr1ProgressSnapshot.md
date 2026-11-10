<!-- generated from Lsr1ProgressSnapshot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::Lsr1ProgressSnapshot`

What an L-SR1 progress hook receives each iteration.

`iteration`, `function_evaluations`, `objective`, `gradient_norm`, `trust_radius`, `step_norm` and a `message`. The hook returns `true` to stop and `false` to keep going. The trust radius shows how the method judges its own model: it shrinks when the model predicted badly and grows when it predicted well.

## Fields

### `iteration`

`std::int64_t`, default `0`.

Iteration number.

### `function_evaluations`

`std::int64_t`, default `0`.

Evaluations so far.

### `objective`

`double`, default `+inf`.

Objective value.

### `gradient_norm`

`double`, default `+inf`.

Gradient 2-norm.

### `trust_radius`

`double`, default `0`.

Trust-region radius.

### `step_norm`

`double`, default `0`.

Step length.

### `message`

`std::string`, default `empty`.

What just happened.

## Guidance

Passed to every L-SR1 `progress_hook`.

## Practical Examples

### Example 1

Recording the radius and step length.

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
  options.max_iterations = 6;
  options.progress_hook = [&out](const optim::Lsr1ProgressSnapshot &s) {
    out.writeln("iteration", s.iteration, ": objective =", s.objective, ", radius =", s.trust_radius, ", step =", s.step_norm);
    return false;
  };
  (void)optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
}
```

Output

```text
iteration 1 : objective = 6.3215 , radius = 1.0000 , step = 0.2500
iteration 2 : objective = 4.1950 , radius = 1.0000 , step = 0.0542
iteration 3 : objective = 4.1138 , radius = 1.0000 , step = 0.0134
iteration 4 : objective = 4.1108 , radius = 1.0000 , step = 0.0016
iteration 5 : objective = 3.9749 , radius = 1.0000 , step = 0.0789
iteration 6 : objective = 3.2889 , radius = 1.0000 , step = 0.5000
```

### Example 2

Stopping when the objective is small enough.

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
  options.progress_hook = [](const optim::Lsr1ProgressSnapshot &s) { return s.objective <= 1e-2; };
  const optim::Lsr1Result result = optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
  out.writeln("exit =", optim::lsr1_exit_flag_name(result.exit_flag), ", objective =", result.objective_value);
}
```

Output

```text
exit = Stopped , objective = 0.0069
```

### Example 3

Counting evaluations as they grow.

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
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (1.0 - a).square() + 100.0 * (b - a.square()).square();
  };

  std::vector<std::int64_t> evaluations;
  optim::Lsr1SolveOptions options;
  options.max_iterations = 5;
  options.progress_hook = [&evaluations](const optim::Lsr1ProgressSnapshot &s) { evaluations.push_back(s.function_evaluations); return false; };
  (void)optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
  out.writeln("evaluations =", evaluations);
}
```

Output

```text
evaluations = [4, 5, 6, 7, 8]
```

## See Also

- [`Lsr1SolveOptions`](Lsr1SolveOptions.md): holds the hook

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Lsr1Options.html">
    <span class="label">Previous</span>
    <span class="title">Lsr1Options</span>
  </a>
  <a class="next-link" href="Lsr1Result.html">
    <span class="label">Next</span>
    <span class="title">Lsr1Result</span>
  </a>
</div>
<!-- apiref:nav:end -->
