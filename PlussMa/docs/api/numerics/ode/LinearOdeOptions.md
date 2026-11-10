<!-- generated from LinearOdeOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::LinearOdeOptions`

Tolerances, step bounds and inner-solver settings of `fluxa_linear`.

The step and tolerance fields have the meaning they have in [`OdeOptions`](OdeOptions.md): the step-doubling error estimate is compared with `absolute_tolerance + relative_tolerance·|y|`. `solve_dims` names the state axes the operator acts on (default the last); leading axes are independent lanes. `linear` is the [`GmresOptions`](../krylov/GmresOptions.md) of the implicit solve at each step.

## Fields

### `steps`

`std::int64_t`, default `100`.

Shared with `OdeOptions` but not a step count here: the adaptive solver chooses its own steps. It must be positive (zero is rejected); leave it at the default.


### `relative_tolerance`

`double`, default `1e-6`.

The relative part of the step-doubling error test `absolute_tolerance + relative_tolerance·|y|`, `1e-6` by default. A step is accepted when one full step and two half steps agree to within it. Tightening it costs steps roughly as its inverse square root, because the underlying midpoint rule is second order.


### `absolute_tolerance`

`double`, default `1e-9`.

The absolute part of the same test, `1e-9` by default. It bounds the error where components of the state are near zero. Scale it to the natural size of the state.


### `initial_step`

`double`, default `0`.

The first step, default 0 which lets the solver estimate one. A first step that is too large is rejected and halved, which costs a solve; supply a value when you know the time scale.


### `minimum_step`

`double`, default `0`.

The smallest step allowed, default 0 (no bound). When the tolerance would need a smaller step the solver throws instead of exceeding the tolerance silently.


### `maximum_step`

`double`, default `+inf`.

The largest step allowed, default infinity. Cap it so that the solver cannot stride across a feature of the operator's time dependence, such as a short pulse in `A(t)`.


### `max_steps`

`std::int64_t`, default `100000`.

The most attempted steps, rejected ones included, default 100000. Exceeding it throws.


### `solve_dims`

`std::vector<std::int64_t>`, default `{-1}`.

The axes of the state that one implicit system acts on, `{-1}` by default (the last axis; every leading axis is an independent lane). Name more axes when the action couples them, for example a 2-D grid state with a stencil that mixes both directions.


### `linear`

`krylov::GmresOptions`, default `{}`.

The settings of the restarted GMRES ([`GmresOptions`](../krylov/GmresOptions.md)) that solves the linear system inside every implicit step. Loosen its tolerance for speed or enlarge `restart` when the operator is hard; the outer error control assumes these solves are accurate.

## Guidance

Passed to `fluxa_linear`.

## Practical Examples

### Example 1

Tolerances. A tighter tolerance takes more steps.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -2.0; };
  for (const double tol : {1e-4, 1e-8}) {
    ode::LinearOdeOptions options;
    options.relative_tolerance = tol;
    const ode::OdeResult r = ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 1.0, options);
    out.writeln("tolerance", tol, ": steps =", r.steps, ", error =", std::abs(r.y_final.items<double>()[0] - std::exp(-2.0)));
  }
}
```

Output

```text
tolerance 0.0001000000 : steps = 14 , error = 0.0001360769
tolerance 0.0000000100 : steps = 260 , error = 0.0000003368
```

### Example 2

A maximum step keeps the solver from striding too far across a feature.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const ode::LinearOdeFn action = [](double t, const Series &y) { return y * std::cos(t); };
  for (const double max_step : {1.0, 0.02}) {
    ode::LinearOdeOptions options;
    options.relative_tolerance = 1e-2;
    options.maximum_step = max_step;
    out.writeln("maximum step", max_step, ": steps =", ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 3.0, options).steps);
  }
}
```

Output

```text
maximum step 1.000 : steps = 6
maximum step 0.020 : steps = 150
```

### Example 3

Lanes. A leading axis solves independent systems together: two lanes with different initial states.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -1.0; };
  const ode::OdeResult r = ode::fluxa_linear(action, pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2}), 0.0, 1.0);
  out.writeln("shape =", r.y_final.sizes(), ", y(1) =", r.y_final.reshape({4}).items<double>());
}
```

Output

```text
shape = [2, 2] , y(1) = [0.367871, 0.735742, 1.103614, 1.471485]
```

### Example 4

The absolute tolerance. The state decays toward zero, where the relative test loses meaning and the absolute one bounds the error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -2.0; };   // y' = -2y

  for (const double atol : {1e-3, 1e-10}) {
    ode::LinearOdeOptions options;
    options.relative_tolerance = 1e-3;
    options.absolute_tolerance = atol;
    out.writeln("absolute tolerance", atol, ": steps =", ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 10.0, options).steps);
  }
}
```

Output

```text
absolute tolerance 0.001000000000 : steps = 12
absolute tolerance 0.000000000100 : steps = 51
```

### Example 5

The initial step. A first step that is too large is rejected and retried, so it appears in the rejected count; a good one is accepted outright.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -2.0; };   // y' = -2y

  for (const double h0 : {0.0, 20.0}) {
    ode::LinearOdeOptions options;
    options.initial_step = h0;
    const ode::OdeResult r = ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 5.0, options);
    out.writeln("initial step", h0, ": steps =", r.steps, ", rejected =", r.rejected_steps);
  }
}
```

Output

```text
initial step 0.0000 : steps = 255 , rejected = 1
initial step 20.0000 : steps = 255 , rejected = 4
```

### Example 6

The minimum step. A tolerance that needs steps smaller than the minimum is refused rather than quietly missed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -2.0; };   // y' = -2y

  ode::LinearOdeOptions options;
  options.relative_tolerance = 1e-12;
  options.minimum_step = 0.5;
  try {
    (void)ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 5.0, options);
    out.writeln("completed");
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::ode::fluxa_linear: required step is below minimum_step
```

### Example 7

The step budget. Too small a budget for the interval throws.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -2.0; };   // y' = -2y

  ode::LinearOdeOptions options;
  options.max_steps = 3;
  options.relative_tolerance = 1e-10;
  try {
    (void)ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 5.0, options);
    out.writeln("completed");
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::ode::fluxa_linear: adaptive step budget exhausted
```

### Example 8

The step count `steps` is part of the shared options but unused by the adaptive solver, except that it must be positive; zero is rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -2.0; };   // y' = -2y

  ode::LinearOdeOptions options;
  options.steps = 0;
  try {
    (void)ode::fluxa_linear(action, pluss::make_series({1.0}), 0.0, 1.0, options);
    out.writeln("completed");
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::ode::fluxa_linear: invalid adaptive-step options
```

### Example 9

The solve axes. A state of shape `[3, 2]` is by default three lanes of a two-component system. Naming both axes makes it one six-component system; for an action that acts elementwise the answers agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const ode::LinearOdeFn action = [](double, const Series &y) { return y * -1.0; };
  const Series y0 = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {3, 2});
  ode::LinearOdeOptions lanes;
  ode::LinearOdeOptions whole;
  whole.solve_dims = {-2, -1};
  const ode::OdeResult a = ode::fluxa_linear(action, y0, 0.0, 1.0, lanes);
  const ode::OdeResult b = ode::fluxa_linear(action, y0, 0.0, 1.0, whole);
  out.writeln("lanes :", a.y_final.reshape({6}).items<double>());
  out.writeln("whole :", b.y_final.reshape({6}).items<double>());
}
```

Output

```text
lanes : [0.367871, 0.735742, 1.103614, 1.471485, 1.839356, 2.207227]
whole : [0.367871, 0.735742, 1.103614, 1.471485, 1.839356, 2.207227]
```

### Example 10

The inner solver. `linear` holds the settings of the GMRES solve inside every implicit step. Because the outer error control assumes those solves are accurate, starving them (one iteration per cycle on an operator that mixes components) makes the step control fail rather than return a poor answer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series M = pluss::make_series({-1.0, 4.0, 1.0, -4.0, -1.0, 0.5, 0.0, 0.0, -2.0}, {3, 3});
  const ode::LinearOdeFn action = [&](double, const Series &y) { return M.matmul(y.unsqueeze(-1)).squeeze(-1); };
  const Series y0 = pluss::make_series({1.0, 0.0, 1.0});
  const ode::OdeResult ok = ode::fluxa_linear(action, y0, 0.0, 1.0);
  out.writeln("default inner solver : y(1) =", ok.y_final.items<double>(), ", steps =", ok.steps);
  ode::LinearOdeOptions starved;
  starved.linear.max_iterations = 1;
  starved.linear.restart = 1;
  try {
    (void)ode::fluxa_linear(action, y0, 0.0, 1.0, starved);
    out.writeln("starved inner solver : completed");
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln("starved inner solver :", text.substr(0, text.find('\n')));
  }
}
```

Output

```text
default inner solver : y(1) = [-0.292083, 0.162552, 0.135334] , steps = 151
starved inner solver : subsp::math::ode::fluxa_linear: adaptive step budget exhausted
```

## See Also

- [`fluxa_linear`](fluxa_linear.md): uses it
- [`GmresOptions`](../krylov/GmresOptions.md): the inner solver's settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="OdeAdjointResult.html">
    <span class="label">Next</span>
    <span class="title">OdeAdjointResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
