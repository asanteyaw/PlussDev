<!-- pluss-hand-authored -->

# Series::CalcAccessor::antidiff.strat

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signatures

```cpp
using IntegrandFn = std::function<Series(const Series& t, const Series& w)>;

// Total integral over the path (the time axis is reduced)
Series Series::CalcAccessor::AntidiffAccessor::strat(
    const IntegrandFn& f, double t0, double t1, std::int64_t dim = 0) const
Series strat(const IntegrandFn& f, const Series& times, std::int64_t dim = 0) const

// Running integral: same shape as the path, zero at the first sample
Series strat_cumulative(const IntegrandFn& f, double t0, double t1, std::int64_t dim = 0) const
Series strat_cumulative(const IntegrandFn& f, const Series& times, std::int64_t dim = 0) const
```

## What It Computes

The Stratonovich integral of `f(t, W_t)` against the path in the receiver:

```text
∫ f(t, W_t) ∘ dW  ≈  Σ ½ [f(t_i, W_i) + f(t_{i+1}, W_{i+1})] (W_{i+1} − W_i)
```

The integrand is read at the average of both ends of each step, where the
[Itô integral](antidiff_ito.md) reads it at the start. The two differ because a
Brownian path has non-vanishing quadratic variation. For `f = W` the Stratonovich
sum is exactly `W_T²/2`, the value the ordinary chain rule gives; the Itô sum is
`(W_T² − QV)/2`. This is why the Stratonovich integral is the one that obeys the
ordinary chain rule, and why it is the right choice for smooth-noise limits and for
integrals against a fractional Brownian motion with Hurst index above one half.

It is the `θ = ½` member of the [`theta`](antidiff_ito.md) family. `strat` is kept as
a named method because it is the common case.

The integrand `f(t, w)` is called **once**, with the time of every grid point and the
path value there stacked along `dim`, and must act pointwise.

For a path that jumps, the average over a jump step is a poor approximation of the
integral along the jump; use [`antidiff.marcus`](antidiff_marcus.md) there.

## Parameters

- The receiver is the path, with `n` samples along `dim`; other axes are independent
  paths, integrated in the same call.
- `f` is the integrand. `t` is the time of each grid point and is broadcastable
  against `w`.
- `t0`, `t1` define a uniform grid with `n` points from `t0` to `t1`.
- `times` is an explicit grid (non-uniform allowed), `n` entries. It only changes the
  times given to `f`; the increments come from the path.
- `dim` is the time axis. The default is 0.

`strat` reduces the time axis. `strat_cumulative` returns the running integral: the
shape of the path, zero at the first sample.

## Examples

Example 1. `∫ W ∘ dW` along a fixed path is exactly `W_T²/2`.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

auto strat = path.calc.antidiff.strat(identity, 0.0, 1.0);

std::cout << "strat " << strat << "  W_T^2/2 = " << 0.4 * 0.4 / 2 << "\n";
```

### Output

```text
strat pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.08  W_T^2/2 = 0.08
```

Example 2. Stratonovich minus Itô is half the quadratic variation of the sampled
path, exactly.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

auto strat = path.calc.antidiff.strat(identity, 0.0, 1.0);
auto ito = path.calc.antidiff.ito(identity, 0.0, 1.0);

std::cout << "strat - ito " << strat.sub(ito) << "\n";
std::cout << "QV/2        " << path.antidiff().quadratic_variation().mul(0.5) << "\n";
```

### Output

```text
strat - ito pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.27
QV/2        pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.27
```

Example 3. The ordinary chain rule: `∫ x² ∘ dx` along a smooth-ish path is close to
`x_T³/3`, which the Itô integral is not.

```cpp
auto dw = pluss::series::randn({500}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 5).mul(std::sqrt(1.0 / 500));
auto path = pluss::Series::zeros({1}, pluss::ScalarType::Float64).unite({dw.cumsum(0)}, 0);
auto square = [](const pluss::Series& t, const pluss::Series& x) { return x.mul(x); };

const double xT = path.select(0, 500).item<double>();
std::cout << "x_T^3/3 " << xT * xT * xT / 3.0 << "\n";
std::cout << "strat   " << path.calc.antidiff.strat(square, 0.0, 1.0) << "\n";
std::cout << "ito     " << path.calc.antidiff.ito(square, 0.0, 1.0) << "\n";
```

### Output

```text
x_T^3/3 0.567627
strat   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.56879041
ito     pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.0789865
```

Example 4. A time-dependent integrand and an explicit non-uniform grid. The
increments are unchanged by the grid; only the times that `f` sees differ.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto f = [](const pluss::Series& t, const pluss::Series& w) { return w.mul(t); };
auto times = pluss::make_series({0.0, 0.1, 0.2, 0.5, 1.0});

std::cout << "uniform grid  " << path.calc.antidiff.strat(f, 0.0, 1.0) << "\n";
std::cout << "explicit grid " << path.calc.antidiff.strat(f, times) << "\n";
```

### Output

```text
uniform grid  pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.0875
explicit grid pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.083
```

Example 5. The running integral starts at zero and ends at the total.

```cpp
auto path = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

std::cout << "running " << path.calc.antidiff.strat_cumulative(identity, 0.0, 1.0) << "\n";
std::cout << "total   " << path.calc.antidiff.strat(identity, 0.0, 1.0) << "\n";
```

### Output

```text
running pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0.045, 0.02, 0, 0.08]
total   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.08
```

Example 6. Many paths in one call, with time on axis 1.

```cpp
auto path = pluss::make_series({{0.0, 0.3, -0.2, 0.0, 0.4}, {0.0, -0.1, 0.2, 0.5, 0.3}});  // [2, 5]
auto identity = [](const pluss::Series& t, const pluss::Series& w) { return w; };

std::cout << "strat per path " << path.calc.antidiff.strat(identity, 0.0, 1.0, 1) << "\n";
std::cout << "W_T^2/2         " << 0.4 * 0.4 / 2 << ", " << 0.3 * 0.3 / 2 << "\n";
```

### Output

```text
strat per path pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.08, 0.045]
W_T^2/2         0.08, 0.045
```

## Gotchas And Failure Modes

- **The integrand is evaluated at both ends of each step.** For a rough integrand
  this averaging is a first-order approximation; refine the grid when accuracy
  matters.
- **Not for processes that jump.** Averaging the two ends of a large jump step is a
  poor approximation of the integral along the jump. Use
  [`antidiff.marcus`](antidiff_marcus.md).
- **The path decides the answer.** The conventions differ by the integrator's own
  quadratic variation, so a path that is not a standard Wiener path gives values that
  are not the standard ones.
- **`f` is vectorized.** A callback that loops over time in C++ defeats the batching.

## Summary

`antidiff.strat` integrates against a sampled path with the integrand read at the
average of both ends of each step. It obeys the ordinary chain rule, and differs
from the Itô integral by half the quadratic variation.

## See Also

- [`antidiff.ito`](antidiff_ito.md): the Itô integral, `theta` and the running forms.
- [`antidiff.marcus`](antidiff_marcus.md): the integral along a path that jumps.
- [`diff.strat`](diff_strat.md): the differential counterpart.
