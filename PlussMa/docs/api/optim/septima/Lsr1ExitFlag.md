<!-- generated from Lsr1ExitFlag.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::Lsr1ExitFlag`

Why a run of `l_sr1_trust_region` stopped.

The same six values as [`SeptimaExitFlag`](SeptimaExitFlag.md): `Converged` (1) when the gradient 2-norm is at most `gradient_tolerance`, `MaxIterations` (0), `Stopped` (−1) for a progress hook returning `true`, `NoProgress` (−2) after `max_no_progress_iterations` without improvement or a step below `step_tolerance`, `NonFiniteEvaluation` (−3) and `InvalidProblem` (−4).

## Fields

### `Converged`

`enumerator`, default `1`.

Gradient norm below tolerance.

### `MaxIterations`

`enumerator`, default `0`.

Budget exhausted.

### `Stopped`

`enumerator`, default `-1`.

A hook returned `true`.

### `NoProgress`

`enumerator`, default `-2`.

No improvement or a vanishing step.

### `NonFiniteEvaluation`

`enumerator`, default `-3`.

NaN or infinity.

### `InvalidProblem`

`enumerator`, default `-4`.

Inconsistent input.

## Guidance

Read as `Lsr1Result::exit_flag`.

## Practical Examples

### Example 1

Converged and budget-limited runs.

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

  optim::Lsr1SolveOptions short_run;
  short_run.max_iterations = 3;
  out.writeln("default :", optim::lsr1_exit_flag_name(optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0})).exit_flag));
  out.writeln("3 iterations:", optim::lsr1_exit_flag_name(optim::l_sr1_trust_region(rosenbrock, pluss::series::make({-1.2, 1.0}), short_run).exit_flag));
}
```

Output

```text
default : Converged
3 iterations: MaxIterations
```

### Example 2

Stopped by a hook.

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

  optim::Lsr1SolveOptions options;
  options.progress_hook = [](const optim::Lsr1ProgressSnapshot &) { return true; };
  const optim::Lsr1Result result = optim::l_sr1_trust_region(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("exit =", optim::lsr1_exit_flag_name(result.exit_flag), "=", static_cast<int>(result.exit_flag), ", message =", result.message);
}
```

Output

```text
exit = Stopped = -1 , message = l_sr1_trust_region: stopped by progress_hook
```

### Example 3

A non-finite objective.

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
  const optim::Lsr1Result result = optim::l_sr1_trust_region([](const Series &x) { return x.log().sum(); }, pluss::series::make({-1.0}));
  out.writeln("exit =", optim::lsr1_exit_flag_name(result.exit_flag));
}
```

Output

```text
exit = NonFiniteEvaluation
```

## See Also

- [`lsr1_exit_flag_name`](lsr1_exit_flag_name.md): names a flag
- [`Lsr1Result`](Lsr1Result.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="LbfgsOptions.html">
    <span class="label">Previous</span>
    <span class="title">LbfgsOptions</span>
  </a>
  <a class="next-link" href="Lsr1Options.html">
    <span class="label">Next</span>
    <span class="title">Lsr1Options</span>
  </a>
</div>
<!-- apiref:nav:end -->
