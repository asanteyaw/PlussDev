# Autodiff API Reference

## Trace construction

### `trace`

#### Signature

```cpp
Trace trace(const Series& value, bool tracks_slope = true)
```

#### Explanation

Wraps a `Series` in reverse-mode tracking state. Set `tracks_slope=false` when the value should flow through the objective without receiving gradients.

#### Example

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}).to(pluss::ScalarType::Float64);
auto tx = pluss::subsp::math::diff::trace(x, true);
```

### `fixed`

#### Signature

```cpp
Trace fixed(const Series& value)
```

#### Explanation

Builds a reverse-mode wrapper that is treated as constant input.

#### Example

```cpp
auto y = pluss::make_series({10.0, 20.0, 30.0}).to(pluss::ScalarType::Float64);
auto fy = pluss::subsp::math::diff::fixed(y);
```

## Trace methods

### `Trace::value`

#### Signature

```cpp
const Series& value(self) const
```

#### Explanation

Returns the primal `Series` currently carried by the trace.

### `Trace::tracks_slope`

#### Signature

```cpp
bool tracks_slope(self) const
```

#### Explanation

Reports whether the trace participates in reverse-mode accumulation.

### `Trace::is_leaf`

#### Signature

```cpp
bool is_leaf(self) const
```

#### Explanation

Returns whether the wrapped value is currently a leaf in the reverse graph.

### `Trace::has_sensitivity`

#### Signature

```cpp
bool has_sensitivity(self) const
```

#### Explanation

Reports whether a reverse pass has already produced a sensitivity for this trace.

### `Trace::sensitivity`

#### Signature

```cpp
Series sensitivity(self) const
```

#### Explanation

Returns the accumulated reverse-mode sensitivity.

### `Trace::keep_sensitivity`

#### Signature

```cpp
void keep_sensitivity(self, bool keep = true)
```

#### Explanation

Requests that the wrapped value retain its sensitivity after reverse propagation.

### `Trace::clear_sensitivity`

#### Signature

```cpp
void clear_sensitivity(self)
```

#### Explanation

Clears any stored reverse-mode sensitivity on the wrapped value.

### `Trace::separate`

#### Signature

```cpp
Trace separate(self) const
```

#### Explanation

Detaches the trace from the current graph while preserving the primal value.

### `Trace::reverse`

#### Signature

```cpp
void reverse(self) const
void reverse(self, const Series& sensitivity_output) const
```

#### Explanation

Runs the reverse pass from this output trace. Supply `sensitivity_output` when the output is non-scalar and you want to seed a custom cotangent.

#### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series(2.0).to(pluss::ScalarType::Float64);
auto tx = trace(x, true);
auto out = square(tx);
out.reverse();

std::cout << tx.sensitivity().item<double>() << "\n";
```

## DualTrace construction

### `trace(value, tangent)`

#### Signature

```cpp
DualTrace trace(const Series& value, const Series& tangent)
```

#### Explanation

Builds a forward-mode dual value whose tangent is propagated alongside the primal.

### `fixed(value, tangent_like)`

#### Signature

```cpp
DualTrace fixed(const Series& value, const Series& tangent_like)
```

#### Explanation

Creates a dual value with zero tangent shaped like `tangent_like`.

## DualTrace methods

### `DualTrace::value`

```cpp
const Series& value(self) const
```

Returns the primal value.

### `DualTrace::tangent`

```cpp
const Series& tangent(self) const
```

Returns the forward tangent.

### `DualTrace::separate`

```cpp
DualTrace separate(self) const
```

Detaches both primal and tangent from the current graph.

## Scalar and elementwise primitives

The following free functions work on both `Trace` and `DualTrace` values and are the intended building blocks for generic autodiff callables:

- `add`, `sub`, `mul`, `div`, `neg`
- `pow`, `exp`, `log`, `sqrt`, `rsqrt`, `square`
- `sin`, `cos`, `tanh`
- `relu`, `sigmoid`, `softplus`, `clamp`
- `sum`, `mean`, `matmul`

Use them directly, or use the overloaded arithmetic operators where available.

### Example

```cpp
using namespace pluss::subsp::math::diff;

auto f = [](const auto& x) {
  return softplus(x, 1.5, 18.0) + sigmoid(x) + square(x);
};
```

## Gradient transforms

### `value_and_grad`

#### Signature

```cpp
std::pair<Series, Series> value_and_grad(Fn&& fn, const Series& param)
std::pair<Series, std::vector<Series>> value_and_grad(Fn&& fn, const std::vector<Series>& params)
```

#### Explanation

Runs the objective once and returns both primal output and gradient.

### `grad`

#### Signature

```cpp
Series grad(Fn&& fn, const Series& param)
std::vector<Series> grad(Fn&& fn, const std::vector<Series>& params)
```

#### Explanation

Returns only the gradient part of `value_and_grad`.

### `jacrev`

#### Signature

```cpp
std::pair<Series, Series> jacrev(Fn&& fn, const Series& param)
std::pair<Series, std::vector<Series>> jacrev(Fn&& fn, const std::vector<Series>& params)
```

#### Explanation

Builds full Jacobians by repeated reverse-mode pullbacks.

### `vjp`

#### Signature

```cpp
std::pair<Series, std::function<Series(const Series&)>> vjp(Fn&& fn, const Series& param)
std::pair<Series, std::function<std::vector<Series>(const Series&)>> vjp(Fn&& fn, const std::vector<Series>& params)
std::pair<Series, Series> vjp(Fn&& fn, const Series& param, const Series& cotangent)
std::pair<Series, std::vector<Series>> vjp(Fn&& fn, const std::vector<Series>& params, const Series& cotangent)
```

#### Explanation

Returns the primal value plus a pullback closure that maps cotangents back to input-space sensitivities.

## Forward-mode transforms

### `jvp`

#### Signature

```cpp
std::pair<Series, Series> jvp(Fn&& fn, const Series& param, const Series& tangent)
std::pair<Series, Series> jvp(Fn&& fn, const std::vector<Series>& params, const std::vector<Series>& tangents)
```

#### Explanation

Computes the directional derivative without materializing a full Jacobian.

### `linearize`

#### Signature

```cpp
std::pair<Series, std::function<Series(const Series&)>> linearize(Fn&& fn, const Series& param)
auto linearize(Fn&& fn, const std::vector<Series>& params)
```

#### Explanation

Returns the primal value and a reusable pushforward closure.

### `jacfwd`

#### Signature

```cpp
std::pair<Series, Series> jacfwd(Fn&& fn, const Series& param)
std::pair<Series, std::vector<Series>> jacfwd(Fn&& fn, const std::vector<Series>& params)
```

#### Explanation

Builds full Jacobians by repeated forward pushforwards.

## Hessian-vector products

### `HvpOptions`

#### Signature

```cpp
struct HvpOptions {
  double epsilon{1e-6};
};
```

#### Explanation

Controls the finite-difference step used by the fallback `hvp` path.

### `hvp`

#### Signature

```cpp
std::pair<Series, Series> hvp(Fn&& fn, const Series& param, const Series& tangent, const HvpOptions& opt = {})
std::pair<Series, std::vector<Series>> hvp(Fn&& fn, const std::vector<Series>& params, const std::vector<Series>& tangents, const HvpOptions& opt = {})
std::pair<Series, Series> vhp(Fn&& fn, const Series& param, const Series& vector, const HvpOptions& opt = {})
std::pair<Series, std::vector<Series>> vhp(Fn&& fn, const std::vector<Series>& params, const std::vector<Series>& vectors, const HvpOptions& opt = {})
```

#### Explanation

Computes Hessian-vector products for scalar-valued objectives.
When the callable is generic over `DualTrace`, the transform takes the exact forward-over-reverse path. Otherwise it falls back to finite-difference-over-grad.
`vhp` is also available and uses the same scalar-objective Hessian product path.

#### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series({0.3, -0.2, 0.5}).to(pluss::ScalarType::Float64);
auto v = pluss::make_series({0.1, 0.05, -0.2}).to(pluss::ScalarType::Float64);

auto [value, hv] = hvp(
    [](const auto& z) {
      return sum(sin(z) + square(z));
    },
    x,
    v);
```

## Derivative audits

### `DerivativeAuditOptions`

#### Signature

```cpp
struct DerivativeAuditOptions {
  double epsilon{1e-6};
  double atol{1e-6};
  double rtol{1e-4};
  bool include_axis_lanes{true};
  HvpOptions hvp{};
  DerivativePolicy policy{};
};
```

### `gradient_audit` and `curvature_audit`

#### Signature

```cpp
bool gradient_audit(Fn&& fn, const Series& param, const DerivativeAuditOptions& opt = {})
bool gradient_audit(Fn&& fn, const std::vector<Series>& params, const DerivativeAuditOptions& opt = {})
bool curvature_audit(Fn&& fn, const Series& param, const DerivativeAuditOptions& opt = {})
bool curvature_audit(Fn&& fn, const std::vector<Series>& params, const DerivativeAuditOptions& opt = {})
DerivativeAuditReport gradient_audit_report(Fn&& fn, const Series& param, const DerivativeAuditOptions& opt = {})
DerivativeAuditReport gradient_audit_report(Fn&& fn, const std::vector<Series>& params, const DerivativeAuditOptions& opt = {})
DerivativeAuditReport curvature_audit_report(Fn&& fn, const Series& param, const DerivativeAuditOptions& opt = {})
DerivativeAuditReport curvature_audit_report(Fn&& fn, const std::vector<Series>& params, const DerivativeAuditOptions& opt = {})
```

#### Explanation

Directional finite-difference audits for first-order and second-order transforms.
These are the Pluss-native equivalents of gradient correctness utilities and are intended for model/solver validation pipelines.
`*_report` variants return lane-level mismatch diagnostics (`phase`, `lane_index`, `analytic`, `numeric`, `abs_error`, `tolerance`).

## Higher-order helper

### `nth_diff`

#### Signature

```cpp
Series nth_diff(Fn&& fn, const Series& param, std::int64_t order, const HvpOptions& opt = {}, const DerivativePolicy& policy = {})
auto nth_diff(Fn&& fn, std::int64_t order, const HvpOptions& opt = {}, const DerivativePolicy& policy = {})
```

#### Explanation

Convenience helper for higher-order derivatives.
For Trace-callable (or Series-callable) objectives, it supports any order `n >= 0`.
DualTrace-only objectives support orders `0`, `1`, and `2`.

For non-scalar outputs, `nth_diff` applies egrad-style behavior at each order:
differentiate the sum of outputs (VJP with all-ones cotangent).
If the objective is disconnected and `strict_dependency=false`, disconnected lanes are
materialized per `DerivativePolicy::disconnected_sensitivity`, and warnings are emitted
by default (`DerivativePolicy::warn_on_disconnected=true`).

#### Example

```cpp
using namespace pluss::subsp::math::diff;

auto x = pluss::make_series(0.4).to(pluss::ScalarType::Float64);
auto second = nth_diff(
    [](const auto& z) {
      return tanh(z);
    },
    x,
    2);

auto d4 = nth_diff(
    [](const auto& z) {
      return exp(z) - z;
    },
    4);
auto fourth = d4(x);

auto xv = pluss::series::linspace(-7.0, 7.0, 700, pluss::ScalarType::Float64);
auto d1v = nth_diff([](const auto& z) { return tanh(z); }, xv, 1);
```
