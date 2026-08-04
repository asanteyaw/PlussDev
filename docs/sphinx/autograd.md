# Autograd Guide

## Includes

```cpp
#include <pluss/pluss.h>
```

For the transform APIs documented below, the public namespace is:

```cpp
using namespace pluss::subsp::math::diff;
```

## What the public surface looks like

PlussQit exposes autodiff in two layers:

- **direct `Series` autograd controls** for eager reverse-mode workflows
- **transform-style APIs** in `pluss::subsp::math::diff` for gradients, Jacobians, vector products, and Hessian-vector products

The direct layer is useful when you already have a scalar loss `Series` and want to call `reverse()`.
The transform layer is what you should prefer for reusable optimization code.

## Smallest working patterns

```cpp
auto x = pluss::make_series(1.25).to(pluss::ScalarType::Float64);

auto tx = trace(x, true);
auto y = tx * tx * tx - 2.0 * (tx * tx) + tx;
y.reverse();
auto g = tx.sensitivity();
```

```cpp
auto [value, grad] = value_and_grad(
    [](const Trace& z) -> Trace { return z * z * z - 2.0 * (z * z) + z; },
    x);
```

Reference demo: `PlussQit/examples/CodeM/autodiff_minimal_demo.cpp`.

## Reverse-mode on plain `Series`

### Signature

```cpp
auto tx = trace(x, true);
auto loss = sum(square(tx));
loss.reverse();
auto g = tx.sensitivity();
```

### Explanation

This is the lowest-level public autodiff path.
You mark a leaf `Series` as tracked, build a scalar loss from it, call `reverse()`, and then read back the sensitivity accumulated on the leaf.

Use this when:

- you are debugging the tape directly
- you want a PyTorch-like eager workflow
- the objective already exists as raw `Series` code

### Example

```cpp
auto x = pluss::make_series(1.25).to(pluss::ScalarType::Float64);
auto tx = trace(x, true);

auto loss = sum(square(tx));
loss.reverse();

auto grad = tx.sensitivity();
std::cout << grad.item<double>() << "\n";   // 2.5
```

## Opt into tracking at construction time

### Signature

```cpp
pluss::series_options()
    .with_dtype(...)
    .with_platform(...)
    .with_tracks_slope(true)
    .with_retain_sensitivity(true);
```

### Explanation

If you want storage and autodiff options set together, use `SeriesOptions`.
This is the clean way to say “create the value on this backend, with this dtype, and track slope from the start”.

### Example

```cpp
auto x = pluss::make_series(
    {0.2, 0.4, 0.6},
    pluss::series_options()
        .with_dtype(pluss::ScalarType::Float64)
        .with_tracks_slope(true)
        .with_retain_sensitivity(true));

auto loss = x.square().mean();
loss.reverse();

std::cout << x.has_sensitivity() << "\n";
std::cout << x.sensitivity() << "\n";
```

## `Trace`: the reverse-mode transform value

### Signature

```cpp
Trace trace(const Series& value, bool tracks_slope = true);
Trace fixed(const Series& value);
```

### Explanation

`Trace` is the reverse-mode wrapper used by transform APIs such as `value_and_grad`, `grad`, `jacrev`, and `vjp`.

Use `trace(x)` for values that should participate in differentiation.
Use `fixed(x)` for values that should flow through the computation without receiving gradients.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);
auto y = pluss::make_series({1.0,  2.0, 3.0}).to(pluss::ScalarType::Float64);

auto tracked = trace(x, true);
auto constant = fixed(y);
auto out = square(tracked) + constant;

out.reverse();
std::cout << tracked.sensitivity() << "\n";
```

## `DualTrace`: the forward-mode transform value

### Signature

```cpp
DualTrace trace(const Series& value, const Series& tangent);
DualTrace fixed(const Series& value, const Series& tangent_like);
```

### Explanation

`DualTrace` carries both the primal value and the tangent.
It powers `jvp`, `jacfwd`, `linearize`, and the exact forward-over-reverse branch of `hvp`.

The `value` and `tangent` must match in dtype, platform, and shape (or be reshape-compatible by total element count).

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);
auto v = pluss::make_series({0.1,  0.05, -0.2}).to(pluss::ScalarType::Float64);

auto dual = trace(x, v);
auto out = sin(dual) + square(dual);

std::cout << out.value() << "\n";
std::cout << out.tangent() << "\n";
```

## `value_and_grad`

### Signature

```cpp
auto [value, grads] = value_and_grad(fn, std::vector<Series>{...});
auto [value, grad]  = value_and_grad(fn, param);
```

### Explanation

`value_and_grad` is the main entry point for gradient-based optimization.
It evaluates the objective once and returns both the primal value and the gradient with respect to each parameter.

Use the single-parameter overload for one trainable `Series`.
Use the vector overload when your objective depends on several parameters.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::series::linspace(-1.0, 1.0, 8, pluss::ScalarType::Float64);
auto y = x.mul(2.0).sub(0.5);

auto w = pluss::make_series(0.0).to(pluss::ScalarType::Float64);
auto b = pluss::make_series(0.0).to(pluss::ScalarType::Float64);

auto [loss, grads] = value_and_grad(
    [&](const std::vector<Trace>& p) -> Trace {
      const Trace pred = p[0] * trace(x, false) + p[1];
      const Trace err = pred - trace(y, false);
      return mean(err * err);
    },
    std::vector<pluss::Series>{w, b});

std::cout << loss.item<double>() << "\n";
std::cout << grads[0].item<double>() << ", " << grads[1].item<double>() << "\n";
```

## `grad`

### Signature

```cpp
auto grads = grad(fn, std::vector<Series>{...});
auto grad  = grad(fn, param);
```

### Explanation

`grad` is the same transform as `value_and_grad`, but returns only the gradient part.
Use it when the primal objective value is irrelevant to the caller.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series(1.25).to(pluss::ScalarType::Float64);

auto g = grad(
    [](const Trace& z) -> Trace {
      return square(z) - 4.0 * z + 1.0;
    },
    x);

std::cout << g.item<double>() << "\n";
```

## `jvp`

### Signature

```cpp
auto [value, directional] = jvp(fn, param, tangent);
auto [value, directional] = jvp(fn, params, tangents);
```

### Explanation

`jvp` computes a Jacobian-vector product.
It tells you how the output changes in one chosen tangent direction without materializing the full Jacobian.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);
auto v = pluss::make_series({0.1,  0.05, -0.2}).to(pluss::ScalarType::Float64);

auto [value, directional] = jvp(
    [](const auto& z) {
      return sin(z) + square(z);
    },
    x,
    v);

std::cout << value << "\n";
std::cout << directional << "\n";
```

## `vjp`

### Signature

```cpp
auto [value, pullback] = vjp(fn, param);
auto [value, pullback] = vjp(fn, params);
```

### Explanation

`vjp` computes a vector-Jacobian product closure.
It first returns the primal output, then gives you a pullback function that maps a cotangent on the output space back into cotangents on the input space.

Use it when the cotangent is not known at the time you evaluate the primal function.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);
auto cotangent = pluss::make_series({1.0, -0.5, 0.25}).to(pluss::ScalarType::Float64);

auto [value, pullback] = vjp(
    [](const Trace& z) -> Trace {
      return sin(z) + square(z);
    },
    x);

auto pulled = pullback(cotangent);
std::cout << value << "\n";
std::cout << pulled << "\n";
```

## `jacrev` and `jacfwd`

### Signature

```cpp
auto [value, jacobian] = jacrev(fn, param);
auto [value, jacobian] = jacfwd(fn, param);
```

### Explanation

Both transforms materialize the full Jacobian.

- `jacrev` builds it row-by-row with reverse-mode pullbacks
- `jacfwd` builds it column-by-column with forward-mode pushforwards

For small vector-output functions, either is fine.
Choose based on the input/output dimensions you expect in the real workload.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);

auto [rev_value, rev_jac] = jacrev(
    [](const auto& z) {
      return sin(z) + square(z);
    },
    x);

auto [fwd_value, fwd_jac] = jacfwd(
    [](const auto& z) {
      return sin(z) + square(z);
    },
    x);

std::cout << rev_jac << "\n";
std::cout << fwd_jac << "\n";
```

## `linearize`

### Signature

```cpp
auto [value, pushforward] = linearize(fn, param);
auto [value, pushforward] = linearize(fn, params);
```

### Explanation

`linearize` evaluates the primal function once and returns a reusable pushforward closure.
This is useful when you want to apply many tangents to the same local linearization without re-running the primal setup each time.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);
auto v1 = pluss::make_series({0.1, 0.05, -0.2}).to(pluss::ScalarType::Float64);
auto v2 = pluss::make_series({-0.3, 0.2, 0.15}).to(pluss::ScalarType::Float64);

auto [value, pushforward] = linearize(
    [](const auto& z) {
      return sin(z) + square(z);
    },
    x);

auto d1 = pushforward(v1);
auto d2 = pushforward(v2);

std::cout << value << "\n";
std::cout << d1 << "\n";
std::cout << d2 << "\n";
```

## `hvp`

### Signature

```cpp
auto [value, hv] = hvp(fn, param, tangent, HvpOptions{});
auto [value, hv] = hvp(fn, params, tangents, HvpOptions{});
```

### Explanation

`hvp` computes a Hessian-vector product for scalar-valued objectives.

Two public execution modes exist:

- exact forward-over-reverse when your callable is written against `DualTrace`
- finite-difference-over-grad fallback when the callable is written only against `Trace`

Set `HvpOptions::epsilon` when you need to tune the finite-difference step in the fallback path.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);
auto v = pluss::make_series({0.1,  0.05, -0.2}).to(pluss::ScalarType::Float64);

HvpOptions opt;
opt.epsilon = 1e-6;

auto [value, hv] = hvp(
    [](const auto& z) {
      return sum(sin(z) + square(z));
    },
    x,
    v,
    opt);

std::cout << value.item<double>() << "\n";
std::cout << hv << "\n";
```

## `nth_diff`

### Signature

```cpp
Series nth_diff(fn, param, std::int64_t order, HvpOptions opt = {}, DerivativePolicy policy = {});
auto nth_diff(fn, std::int64_t order, HvpOptions opt = {}, DerivativePolicy policy = {});
```

### Explanation

`nth_diff` is the higher-order convenience helper for any order `n >= 0`
when the objective is Trace-callable (or Series-callable with traced input).

For non-scalar outputs, `nth_diff` applies egrad-style behavior at each order:
it differentiates the sum of outputs (VJP with an all-ones cotangent).
When the Jacobian is diagonal (elementwise vectorized scalar functions), this
matches elementwise derivatives exactly.

You can call it directly with `(fn, param, n)` or build a callable derivative
transform with `(fn, n)` and apply it repeatedly on scalar inputs.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series(1.25).to(pluss::ScalarType::Float64);

auto second = nth_diff(
    [](const auto& z) {
      return z * z - 4.0 * z + 1.0;
    },
    x,
    2);

auto d3 = nth_diff(
    [](const auto& z) {
      return sin(z) + square(z);
    },
    3);

auto third = d3(x);

std::cout << second.item<double>() << "\n";
std::cout << third.item<double>() << "\n";
```

### More `nth_diff` patterns

```cpp
using namespace pluss::subsp::math::diff;
auto x = pluss::make_series(0.7).to(pluss::ScalarType::Float64);
```

Direct first derivative:

```cpp
auto d1 = nth_diff([](const auto& z) { return exp(z); }, x, 1);
```

Vectorized input (elementwise function over many values):

```cpp
auto xv = pluss::series::linspace(-7.0, 7.0, 700, pluss::ScalarType::Float64);
auto d1v = nth_diff([](const auto& z) { return tanh(z); }, xv, 1);
auto d2v = nth_diff([](const auto& z) { return tanh(z); }, xv, 2);
```

Direct fourth derivative:

```cpp
auto d4 = nth_diff([](const auto& z) { return z * z * z * z; }, x, 4);
```

Zero-order returns the value:

```cpp
auto value = nth_diff([](const auto& z) { return sin(z) + z; }, x, 0);
```

Callable transform (egrad-like):

```cpp
auto grad_fn = nth_diff([](const auto& z) { return square(z); }, 1);
auto g = grad_fn(x);
```

Callable second-derivative transform:

```cpp
auto hess_diag_fn = nth_diff([](const auto& z) { return tanh(z); }, 2);
auto h = hess_diag_fn(x);
```

Pass a normal `Series` function (not explicitly Trace-typed):

```cpp
auto f_series = [](const pluss::Series& z) { return sin(z) * z; };
auto d2 = nth_diff(f_series, x, 2);
```

Use strict dependency behavior:

```cpp
DerivativePolicy pol{};
pol.strict_dependency = true;
auto d = nth_diff([](const auto& z) { return z * 0.0; }, x, 1, {}, pol); // throws
```

Use disconnected-as-undefined behavior:

```cpp
DerivativePolicy pol{};
pol.disconnected_sensitivity = DisconnectedSensitivity::Undefined;
auto d = nth_diff([](const auto& z) { return z * 0.0; }, x, 1, {}, pol); // undefined Series
```

Disable disconnected warnings (default is `true`):

```cpp
DerivativePolicy pol{};
pol.warn_on_disconnected = false;
auto d = nth_diff([](const auto& z) { return z * 0.0; }, x, 1, {}, pol); // zero/undefined per policy, no warning
```

From the optimization facade namespace:

```cpp
namespace pmo = pluss::optim;
auto d3_opt = pmo::nth_diff([](const auto& z) { return z * z * z; }, x, 3);
```

## Retaining or clearing sensitivities

### Signature

```cpp
x.retain_grad(true);
x.retains_grad();
x.clear_sensitivity();
```

### Explanation

Use these on raw `Series` when you need to keep or clear accumulated sensitivities manually.
They matter most in long-running eager workflows where you do not want stale sensitivities from the previous reverse pass to leak into the next one.

### Example

```cpp
auto x = pluss::make_series(2.0).to(pluss::ScalarType::Float64);
auto tx = trace(x, true);
x.retain_grad(true);

auto loss = square(tx);
loss.reverse();
std::cout << x.retains_grad().item<bool>() << "\n";
std::cout << x.sensitivity().item<double>() << "\n";

x.clear_sensitivity();
std::cout << x.has_sensitivity() << "\n";
```

## Mutation safety and version counters

Tracked in-place mutation is intentionally restricted.
Pluss bumps a storage version on in-place writes and blocks certain mutations on tracked values.
That protects the reverse pass from silently reading stale saved state.

```cpp
auto x = pluss::make_series(1.0).to(pluss::ScalarType::Float64);
auto before = x.storage_version();
x.relu_();
auto after = x.storage_version();
std::cout << before << " -> " << after << "\n";

auto tracked = pluss::make_series(0.25).to(pluss::ScalarType::Float64);
(void)trace(tracked, true);

try {
  tracked.add_(1.0);
} catch (const std::exception& ex) {
  std::cout << ex.what() << "\n";
}
```

## Next page

Use the `Autograd API Reference` tab when you want the direct signature-by-signature surface.
