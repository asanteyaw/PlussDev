<!-- pluss-hand-authored -->

# Series::CalcAccessor::antidiff.ito

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signatures

```cpp
using IntegrandFn = std::function<Series(const Series& t, const Series& w)>;

// Total integral over the path (the time axis is reduced)
Series Series::CalcAccessor::AntidiffAccessor::ito(
    const IntegrandFn& f, double t0, double t1, std::int64_t dim = 0) const
Series ito(const IntegrandFn& f, const Series& times, std::int64_t dim = 0) const

// Evaluation point inside each step: (1 - theta) f(left) + theta f(right)
Series theta(const IntegrandFn& f, double theta, double t0, double t1, std::int64_t dim = 0) const
Series theta(const IntegrandFn& f, double theta, const Series& times, std::int64_t dim = 0) const

// Running integral: same shape as the path, zero at the first sample
Series ito_cumulative(const IntegrandFn& f, double t0, double t1, std::int64_t dim = 0) const
Series ito_cumulative(const IntegrandFn& f, const Series& times, std::int64_t dim = 0) const
```

`strat` and `strat_cumulative`, the Stratonovich forms, have their own page:
[`antidiff.strat`](antidiff_strat.md). They appear below only for comparison.

## What It Computes

The receiver is a **path**: a Brownian path (or any integrator) sampled on a time
grid, with the time axis `dim`. `antidiff.ito(f, t0, t1)` integrates `f(t, W_t)`
against the path:

```text
∫ f(t, W_t) dW  ≈  Σ f(t_i, W_i) (W_{i+1} − W_i)          (Itô)
∫ f(t, W_t) ∘ dW ≈  Σ ½ [f(t_i, W_i) + f(t_{i+1}, W_{i+1})] (W_{i+1} − W_i)   (Stratonovich)
```

The two differ because a Brownian path has non-vanishing quadratic variation: the
Itô sum reads the integrand at the start of each step, the Stratonovich sum at the
average of both ends, and for `f = W` they differ by half the quadratic variation
of the path, however fine the grid. `theta` interpolates: `θ = 0` is Itô, `θ = ½` is
Stratonovich, and `θ = 1` is the backward (anticipating) convention.

The integrand `f(t, w)` is called **once**, with the time of every grid point and
the path value there stacked along `dim`, and must act pointwise. Because it never
sees any other sample, a pointwise integrand cannot anticipate the path.

## Parameters

- The receiver is the path, with `n` samples along `dim`; other axes are
  independent paths, integrated in the same call.
- `f` is the integrand. `t` is the time of each grid point and is broadcastable
  against `w`.
- `t0`, `t1` define a uniform grid with `n` points from `t0` to `t1`.
- `times` is an explicit grid (non-uniform allowed), `n` entries. It only changes
  the times given to `f`; the increments come from the path.
- `theta` is the evaluation weight (0 for Itô, 0.5 for Stratonovich, 1 for backward).
- `dim` is the time axis. The default is 0.

The total forms reduce the time axis. The `_cumulative` forms return the running
integral: the shape of the path, zero at the first sample.

## Examples

Example 1. Itô and Stratonovich for `∫ W dW` along a fixed path. The Stratonovich
value is exactly `W_T²/2`; the Itô value is `(W_T² − QV)/2`, where `QV` is the
quadratic variation of the sampled path.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

auto ito = path.calc.antidiff.ito(identity, 0.0, 1.0);
auto strat = path.calc.antidiff.strat(identity, 0.0, 1.0);

const double wT = 0.4, qv = 0.09 + 0.25 + 0.04 + 0.16;
std::cout << "ito   " << ito << "  (W_T^2 - QV)/2 = " << (wT * wT - qv) / 2 << "\n";
std::cout << "strat " << strat << "  W_T^2/2 = " << wT * wT / 2 << "\n";
```

### Output

```text
ito   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.19  (W_T^2 - QV)/2 = -0.19
strat pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.08  W_T^2/2 = 0.08
```

Example 2. The theta family. Each value is `W_T²/2 + c · QV/2` with `c = −1, 0, +1`
for `θ = 0, ½, 1`.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

for (double theta : {0.0, 0.5, 1.0}) {
  std::cout << "theta " << theta << ": " << path.calc.antidiff.theta(identity, theta, 0.0, 1.0).item<double>() << "\n";
}
std::cout << "W_T^2/2 = 0.08, QV/2 = 0.27\n";
```

### Output

```text
theta 0: -0.19
theta 0.5: 0.08
theta 1: 0.35
W_T^2/2 = 0.08, QV/2 = 0.27
```

Example 3. A time-dependent integrand, and an explicit non-uniform grid. The
increments of the path are unchanged by the grid; only the times that `f` sees
differ.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto f = [](const pluss::Series& t, const pluss::Series& w) { return w.mul(t); };

auto uniform = path.calc.antidiff.ito(f, 0.0, 1.0);
auto times = pluss::make_series({0.0, 0.1, 0.2, 0.5, 1.0});
auto warped = path.calc.antidiff.ito(f, times);

std::cout << "uniform grid " << uniform << "\n";
std::cout << "explicit grid " << warped << "\n";
```

### Output

```text
uniform grid pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.0575
explicit grid pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.023
```

Example 4. The running integral. It has the shape of the path, starts at zero, and
ends at the total.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

auto running = path.calc.antidiff.ito_cumulative(identity, 0.0, 1.0);
auto total = path.calc.antidiff.ito(identity, 0.0, 1.0);

std::cout << "running " << running << "\n";
std::cout << "total   " << total << "\n";
```

### Output

```text
running pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, -0.15, -0.19, -0.19]
total   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.19
```

Example 5. Many paths in one call. Time is on axis 0, paths on axis 1; the result
has one value per path.

```cpp
auto dw = pluss::series::randn({400, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 3).mul(0.05);
auto paths = pluss::Series::zeros({1, 3}, pluss::ScalarType::Float64).unite({dw.cumsum(0)}, 0);  // [401, 3]
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

auto ito = paths.calc.antidiff.ito(identity, 0.0, 1.0);
auto strat = paths.calc.antidiff.strat(identity, 0.0, 1.0);

std::cout << "ito   " << ito << "\n";
std::cout << "strat - ito = QV/2 = " << strat.sub(ito) << "\n";
```

### Output

```text
ito   pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.33957332, -0.13499971, -0.25184846]
strat - ito = QV/2 = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.45849474, 0.51223781, 0.54184003]
```

Example 6. Time on the last axis: pass `dim`.

```cpp
auto path = pluss::make_series({{0.0, 0.3, -0.2, 0.0, 0.4}, {0.0, -0.1, 0.2, 0.5, 0.3}});  // [2, 5]
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

auto ito = path.calc.antidiff.ito(identity, 0.0, 1.0, 1);

std::cout << "ito per path " << ito << "\n";
```

### Output

```text
ito per path pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.19, -0.07]
```

Example 7. A grid of the wrong length is rejected.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };
auto times = pluss::make_series({0.0, 0.5, 1.0});

try {
  std::cout << path.calc.antidiff.ito(identity, times) << "\n";
} catch (const std::exception& e) {
  std::string message = e.what();
  std::cout << "error: " << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
error: Series.calc.antidiff (stochastic integral): the time grid must be one-dimensional with one entry per path sample
```

## Gotchas And Failure Modes

- **Adaptedness cannot be checked here.** A pointwise integrand cannot look at
  other samples, but a callback that captures a copy of the whole path can. An Itô
  integral is only meaningful for an adapted integrand, and a `Series` has no way
  to verify that. The `subsp::math::sde` helpers in the numerical-methods library
  check it before forwarding to this method.
- **The path decides the answer.** The values mean what their names say only when
  the integrator is a standard Wiener path, whose quadratic variation equals the
  elapsed time. For another integrator the conventions still differ, but by that
  integrator's own quadratic variation.
- **`f` is vectorized.** It is called once on the stacked grid. A callback that
  loops over time in C++ defeats the batching and is much slower.

## Summary

`antidiff.ito` and `antidiff.strat` integrate a function of time and path against a
sampled path, in the Itô and Stratonovich conventions; `theta` interpolates and the
`_cumulative` forms return the running integral.

## See Also

- [`antidiff.strat`](antidiff_strat.md): the Stratonovich integral.
- [`antidiff.marcus`](antidiff_marcus.md): the integral along a path that jumps.
- [`antidiff` (sampled)](../series-antidiff/stochastic_integrals.md): the same sums
  for an integrand that is already sampled.
- [`diff.ito`](diff_ito.md): the differential counterpart.
