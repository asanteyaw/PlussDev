<!-- pluss-hand-authored -->

# Series::CalcAccessor::antidiff

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Related Accessor Methods

`x.calc.antidiff` is also an accessor with named methods for stochastic integrals
along a path: `ito`, `strat`, `theta`, `marcus` and their running forms. They are
documented on their own pages, linked under See Also. The call form on this page is
unchanged.

## Signatures

```cpp
template<class Fn>
Series Series::CalcAccessor::antidiff(Fn&& fn) const

template<class Fn>
Series Series::CalcAccessor::antidiff(Fn&& fn, double lower) const

template<class Fn>
Series Series::CalcAccessor::antidiff(
    Fn&& fn,
    double lower,
    const Series::CalcAccessor::CalcOptions& options) const
```

## What It Computes

`antidiff()` evaluates an anchored integral at every value of the owning
`Series`. For first order, receiver `u`, lower bound `a`, and callback `g`, the
result is

```text
F(u) = integral from a to u of g(t) dt + F(a)
```

The callback is the integrand, not a precomputed sample array. The method
chooses quadrature nodes, passes all nodes to `fn` in one batched `Series`, and
combines the returned values with Simpson or trapezoidal weights. This is the
callable counterpart to sampled-data
[`Series::AntidiffAccessor::trapezoid`](../series-antidiff/trapezoid.md), not an
alias for it.

The receiver supplies upper endpoints. A scalar receiver produces a scalar;
a vector, matrix, or higher-rank receiver produces one integral per element and
keeps its shape. A value below `lower` is valid: node spacing becomes negative,
so the integral has the mathematically correct reversed orientation.

## Repeated Integration

`options.order = n` requests the `n`th anchored antiderivative. The engine uses
the weighted identity

```text
F_n(u) = sum(k=0..n-1) F_n^(k)(a) * (u-a)^k / k!
       + integral from a to u of (u-t)^(n-1) * g(t) / (n-1)! dt
```

rather than recursively nesting `n` independent quadrature calls. This keeps
endpoint lanes batched and evaluates the integrand once per refinement level.

`initial[k]` is `F_n^(k)(lower)` for `k = 0 .. n-1`. An empty `initial` means
all those conditions are zero. Supplying some but not all conditions is an
error because it would leave the requested antiderivative under-specified.

## Callback Shape Contract

If the receiver shape is `U` and one estimate uses `m` quadrature nodes, `fn`
receives shape `U + [m]`. It must return exactly that shape on the same
platform. Write the callback entirely with shape-preserving `Series`
operations:

```cpp
auto g = [](const pluss::Series& t) {
  return t.square().mul((-t).exp());
};
```

Do not reduce the node axis inside the callback; `antidiff()` owns that
reduction. All returned values must be finite. A singularity sampled on the
closed interval is reported instead of allowing `NaN` or infinity to poison
the estimate.

## Quadrature And Convergence

`Rule::Simpson` is the default and rounds an odd interval request upward to an
even count, with a minimum of two. `Rule::Trapezoid` accepts any positive
interval count. With `adaptive=true`, the method doubles the interval count
until

```text
max(abs(refined - previous)) <= atol + rtol * max(abs(refined))
```

or `max_refinements` is exhausted. `require_convergence=true` turns exhaustion
into an error; `false` returns the last estimate. The error estimate controls
quadrature refinement only. It is not a proof that the callback is smooth or
that a sampled singularity does not exist between nodes.

## Options

| Field | Meaning | Default |
| --- | --- | --- |
| `order` | Number of anchored integrations | `1` |
| `lower` | Bound used by the no-bound overload | unset |
| `initial` | `F^(k)(lower)` for `k=0..order-1` | all zero |
| `rule` | `Rule::Simpson` or `Rule::Trapezoid` | `Rule::Simpson` |
| `intervals` | Starting interval count | `256` |
| `adaptive` | Double intervals until tolerance is met | `true` |
| `atol` | Absolute convergence tolerance | `1e-10` |
| `rtol` | Relative convergence tolerance | `1e-8` |
| `max_refinements` | Maximum interval doublings | `8` |
| `require_convergence` | Throw if refinement exhausts its budget | `true` |

`elementwise` and `strict_dependency` belong to differentiation and are ignored
here.

## Examples

Example 1. One vectorized call integrates four upper endpoints and preserves
their matrix shape.

```cpp
auto upper = pluss::make_series({{0.5, 1.0}, {1.5, 2.0}});

pluss::Series::CalcAccessor::CalcOptions options;
options.intervals = 32;
options.adaptive = false;

auto area = upper.calc.antidiff(
    [](const pluss::Series& t) { return t.square(); },
    0.0,
    options);

std::cout << area << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.041666667, 0.33333333],
  [1.125, 2.6666667]]
```

Example 2. An upper endpoint below the lower bound reverses orientation. Here
`integral_0^-1 t dt = 1/2`.

```cpp
auto upper = pluss::make_series(-1.0);
std::cout << upper.calc.antidiff(
    [](const pluss::Series& t) { return t; }, 0.0) << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.5
```

Example 3. A second antiderivative needs two lower-bound conditions. For
`F''(u)=1`, `F(0)=2`, and `F'(0)=3`, the result is
`F(u)=2+3u+u^2/2`.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.order = 2;
options.initial = {2.0, 3.0};
options.intervals = 32;
options.adaptive = false;

auto upper = pluss::make_series({{0.5, 1.0}, {1.5, 2.0}});
auto result = upper.calc.antidiff(
    [](const pluss::Series& t) { return t.mul(0.0).add(1.0); },
    0.0,
    options);

std::cout << result << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3.625, 5.5],
  [7.625, 10]]
```

Example 4. Empty initial conditions do not infer constants; they explicitly
select the zero-anchored member of the antiderivative family.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.order = 2;
options.intervals = 32;
options.adaptive = false;

auto upper = pluss::make_series({{0.5, 1.0}, {1.5, 2.0}});
options.initial.clear();
std::cout << upper.calc.antidiff(
    [](const pluss::Series& t) { return t.mul(0.0).add(1.0); },
    0.0,
    options) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.125, 0.5],
  [1.125, 2]]
```

Example 5. Fixed Simpson and trapezoidal estimates expose the accuracy tradeoff
without adaptive refinement. Both use eight intervals for `integral_0^1 t^4
dt`, whose exact value is `0.2`.

```cpp
using Calc = pluss::Series::CalcAccessor;
auto upper = pluss::make_series(1.0);

Calc::CalcOptions trap;
trap.rule = Calc::Rule::Trapezoid;
trap.intervals = 8;
trap.adaptive = false;

Calc::CalcOptions simpson = trap;
simpson.rule = Calc::Rule::Simpson;

auto fourth = [](const pluss::Series& t) { return t.pow(4.0); };
std::cout << std::setprecision(10);
std::cout << "trap=" << upper.calc.antidiff(fourth, 0.0, trap).item<double>() << "\n";
std::cout << "simpson=" << upper.calc.antidiff(fourth, 0.0, simpson).item<double>() << "\n";
```

### Output

```text
trap=0.2052001953
simpson=0.2000325521
```

Example 6. Adaptive Simpson refinement resolves a smooth oscillatory integrand.

```cpp
auto upper = pluss::make_series(std::numbers::pi);
auto area = upper.calc.antidiff(
    [](const pluss::Series& t) { return t.sin(); }, 0.0);

std::cout << area.item<double>() << "\n";
```

### Output

```text
2
```

Example 7. Binding a lower bound and policy avoids repeating configuration;
an explicit lower bound still wins for that call.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.lower = 0.0;
options.intervals = 32;
options.adaptive = false;

auto upper = pluss::make_series({0.5, 1.0, 1.5, 2.0});
auto configured = upper.calc(options);
auto square = [](const pluss::Series& t) { return t.square(); };

std::cout << "bound=" << configured.antidiff(square) << "\n";
std::cout << "override=" << configured.antidiff(square, 1.0) << "\n";
```

### Output

```text
bound=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.041666667, 0.33333333, 1.125, 2.6666667]
override=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-0.29166667, 0, 0.79166667, 2.3333333]
```

Example 8. The upper endpoint remains differentiable. The derivative of an
anchored integral with respect to its upper endpoint is the integrand evaluated
there.

```cpp
auto upper = pluss::make_series(0.75);
upper.tracks_slope_(true);

auto area = upper.calc.antidiff(
    [](const pluss::Series& t) { return t.square(); }, 0.0);
area.reverse();

std::cout << "area=" << area.item<double>() << "\n";
std::cout << "darea_dupper=" << upper.sensitivity().item<double>() << "\n";
```

### Output

```text
area=0.140625
darea_dupper=0.5625
```

## Empty Inputs

An empty real floating receiver returns an empty clone with the same shape,
dtype, and platform. No callback evaluation is needed because there are no
endpoints.

## Failure Modes

- The receiver must be defined and have a real floating dtype.
- `lower` must be finite. The no-bound overload requires `options.lower`.
- `order` must be at least one; `intervals` must be positive.
- `atol` and `rtol` must be finite and non-negative.
- `initial` must be empty or contain exactly `order` values.
- `rule` must be `Rule::Simpson` or `Rule::Trapezoid`.
- The callback must return a defined, finite `Series` with the exact node shape
  and platform it received.
- Adaptive exhaustion throws when `require_convergence` is true.
- Doubling an interval count that would overflow `int64` throws before the
  multiplication.

Representative diagnostics are stable:

```text
Series.calc.antidiff: no lower bound; pass one or bind it with x.calc({.lower = a})
Series.calc.antidiff: integrand output shape must match the quadrature-node shape
Series.calc.antidiff: initial must be empty or contain one value per order
Series.calc.antidiff: rule is not recognized
```

## See Also

- [`Series::CalcAccessor::options`](options.md): complete option ownership and
  override rules.
- [`Series::CalcAccessor::diff`](diff.md): callable differentiation at the
  receiver.
- Stochastic integrals along a path: [`antidiff.ito`](antidiff_ito.md),
  [`antidiff.strat`](antidiff_strat.md), [`antidiff.marcus`](antidiff_marcus.md).
- [`Series::AntidiffAccessor::trapezoid`](../series-antidiff/trapezoid.md):
  integrate values that have already been sampled.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_calc_accessor_api_reference.html"><span class="label">Previous</span><span class="title">CalcAccessor</span></a>
  <a class="next-link" href="diff.html"><span class="label">Next</span><span class="title">diff</span></a>
</div>
