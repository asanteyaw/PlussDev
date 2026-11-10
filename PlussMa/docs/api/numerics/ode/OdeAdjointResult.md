<!-- generated from OdeAdjointResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `ode::OdeAdjointResult`

The final state and the gradients returned by the adjoint solve.

`y_final` is the state at `t1`; `grad_y0` is `∂L/∂y₀`; `grad_theta` is `∂L/∂θ`, undefined when no parameters were passed; `forward_steps` and `backward_steps` count the steps of the two passes (both equal `options.steps`).

## Fields

### `y_final`

`pluss::Series`, default `undefined`.

The state at `t1` from the forward sweep, the same as `fluxa` would return.


### `grad_y0`

`pluss::Series`, default `undefined`.

The gradient `∂L/∂y₀` of the scalar loss `L = wᵀ y(t1)` with respect to the initial state, shaped like `y0`. It is the exact gradient of the discrete RK4 map, not an approximation of the continuous one.


### `grad_theta`

`pluss::Series`, default `undefined`.

The gradient `∂L/∂θ` with respect to the parameters, shaped like `theta`; undefined when no parameters were passed.


### `forward_steps`

`std::int64_t`, default `0`.

The steps of the forward sweep, which equals `OdeOptions::steps`.


### `backward_steps`

`std::int64_t`, default `0`.

The steps of the backward adjoint sweep, equal to the forward count; the checkpointed adjoint retraces exactly the steps it took.

## Guidance

Returned by `fluxa_adjoint`.

## Practical Examples

### Example 1

Reading a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const ode::OdeFn decay = [](double, const Series &y, const Series &theta) { return y * theta * -1.0; };
  const ode::OdeAdjointResult r = ode::fluxa_adjoint(decay, pluss::make_series({1.0}), 0.0, 1.0, pluss::make_series({1.0}), pluss::make_series({1.0}));
  out.writeln("y_final =", r.y_final.items<double>(), ", grad_y0 =", r.grad_y0.items<double>(), ", grad_theta =", r.grad_theta.items<double>());
  out.writeln("forward steps =", r.forward_steps, ", backward steps =", r.backward_steps);
}
```

Output

```text
y_final = [0.36787944] , grad_y0 = [0.36787944] , grad_theta = [-0.36787944]
forward steps = 100 , backward steps = 100
```

### Example 2

A different seed scales the gradients: the gradient of `3 y(1)` is three times that of `y(1)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const ode::OdeFn decay = [](double, const Series &y, const Series &theta) { return y * theta * -1.0; };
  for (const double seed : {1.0, 3.0}) {
    const ode::OdeAdjointResult r = ode::fluxa_adjoint(decay, pluss::make_series({1.0}), 0.0, 1.0, pluss::make_series({1.0}), pluss::make_series({seed}));
    out.writeln("seed", seed, ": dL/dtheta =", r.grad_theta.items<double>());
  }
}
```

Output

```text
seed 1.00000000 : dL/dtheta = [-0.36787944]
seed 3.00000000 : dL/dtheta = [-1.10363832]
```

### Example 3

A vector parameter. The gradient has the shape of `theta`: for `y' = −θ₀ y₀, −θ₁ y₁` each component decays at its own rate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace ode = pluss::subsp::math::ode;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const ode::OdeFn decay = [](double, const Series &y, const Series &theta) { return y * theta * -1.0; };
  const ode::OdeAdjointResult r = ode::fluxa_adjoint(decay, pluss::make_series({1.0, 2.0}), 0.0, 1.0, pluss::make_series({0.5, 1.5}), pluss::make_series({1.0, 1.0}));
  out.writeln("grad_theta =", r.grad_theta.items<double>(), ", exact =", -std::exp(-0.5), -2.0 * std::exp(-1.5));
}
```

Output

```text
grad_theta = [-0.606531, -0.446260] , exact = -0.606531 -0.446260
```

## See Also

- [`fluxa_adjoint`](fluxa_adjoint.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="LinearOdeOptions.html">
    <span class="label">Previous</span>
    <span class="title">LinearOdeOptions</span>
  </a>
  <a class="next-link" href="OdeEventDirection.html">
    <span class="label">Next</span>
    <span class="title">OdeEventDirection</span>
  </a>
</div>
<!-- apiref:nav:end -->
