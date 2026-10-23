<!-- pluss-hand-authored -->

# Series::AntidiffAccessor::ito

[Back to Series::AntidiffAccessor API Reference](../../series_antidiff_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesAntidiffProxy::ito(std::int64_t dim = -1) const
Series SeriesAntidiffProxy::ito_cumulative(std::int64_t dim = -1) const
Series SeriesAntidiffProxy::strat(std::int64_t dim = -1) const
Series SeriesAntidiffProxy::strat_cumulative(std::int64_t dim = -1) const
Series SeriesAntidiffProxy::theta(double theta, std::int64_t dim = -1) const
```

These methods are reached through the proxy: `f.antidiff(w).ito()`. The receiver `f`
is the integrand and the peer `w` is the integrator path.

## What It Does

They are the sampled-data counterparts of
[`calc.antidiff.ito`](../series-calc/antidiff_ito.md). There the integrand is a
function; here it is a `Series` already evaluated on the path. The receiver holds the
integrand at every grid point (`n` samples along `dim`, or `n − 1` for `ito`, which
reads only the left end of each step); the peer `w` holds the path (`n` samples).

| Call | Sum |
| --- | --- |
| `f.antidiff(w).ito()` | `Σ f_i (w_{i+1} − w_i)` |
| `f.antidiff(w).strat()` | `Σ ½ (f_i + f_{i+1}) (w_{i+1} − w_i)` |
| `f.antidiff(w).theta(θ)` | `Σ [(1 − θ) f_i + θ f_{i+1}] (w_{i+1} − w_i)` |

`θ = 0` is Itô, `θ = ½` Stratonovich, `θ = 1` the backward convention. The
`_cumulative` forms return the running integral: the length of the path, zero at the
first sample.

## Parameters

- The receiver is the sampled integrand.
- The peer path `w` is bound with `f.antidiff(w)`. Without a peer these methods
  throw.
- `theta` is the evaluation weight.
- `dim` is the time axis. The default is the last axis.

## Examples

Example 1. Itô and Stratonovich sums of `∫ W dW`, with the integrand sampled from the
path itself.

```cpp
auto w = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});

auto ito = w.antidiff(w).ito();
auto strat = w.antidiff(w).strat();

std::cout << "ito   " << ito << "\n";
std::cout << "strat " << strat << "\n";
```

### Output

```text
ito   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.19
strat pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.08
```

Example 2. The integrand `x²` against a sampled `x`. The Stratonovich value is close
to `x_T³/3`; the Itô value is not.

```cpp
auto x = pluss::Series::linspace(0.0, 1.5, 101, pluss::ScalarType::Float64).add(
    pluss::series::randn({101}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 4).mul(0.05));
auto f = x.mul(x);

std::cout << "ito   " << f.antidiff(x).ito() << "\n";
std::cout << "strat " << f.antidiff(x).strat() << "\n";
std::cout << "x_T^3/3 = " << std::pow(x.select(0, 100).item<double>(), 3) / 3.0 << "\n";
```

### Output

```text
ito   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.76363968
strat pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.1511589
x_T^3/3 = 1.15063
```

Example 3. The theta family on the same data.

```cpp
auto w = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});

for (double theta : {0.0, 0.5, 1.0}) {
  std::cout << "theta " << theta << ": " << w.antidiff(w).theta(theta).item<double>() << "\n";
}
```

### Output

```text
theta 0: -0.19
theta 0.5: 0.08
theta 1: 0.35
```

Example 4. The running sums.

```cpp
auto w = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});

std::cout << "ito running   " << w.antidiff(w).ito_cumulative() << "\n";
std::cout << "strat running " << w.antidiff(w).strat_cumulative() << "\n";
```

### Output

```text
ito running   pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, -0.15, -0.19, -0.19]
strat running pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0.045, 0.02, 0, 0.08]
```

Example 5. Several paths: time on axis 0.

```cpp
auto w = pluss::make_series({{0.0, 0.0}, {0.3, -0.1}, {-0.2, 0.2}, {0.0, 0.5}, {0.4, 0.3}});  // [5, 2]

std::cout << "ito per path " << w.antidiff(w).ito(0) << "\n";
```

### Output

```text
ito per path pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.19, -0.07]
```

Example 6. Without a peer path the call throws.

```cpp
auto f = pluss::make_series({0.0, 0.3, -0.2});

try {
  std::cout << f.antidiff().ito() << "\n";
} catch (const std::exception& e) {
  std::string message = e.what();
  std::cout << "error: " << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
error: Series.antidiff(...): this operation requires one peer Series (the x coordinates)
```

## Gotchas And Failure Modes

- The sums only mean what their names say when `f_i` was computed from the path up
  to sample `i` (an adapted integrand). A `Series` cannot verify that.
- For a non-Brownian integrator the Itô and Stratonovich conventions still differ,
  by that integrator's quadratic variation.

## Summary

Sampled stochastic integrals: `f.antidiff(w).ito()`, `.strat()` and `.theta(θ)`,
with running forms.

## See Also

- [`calc.antidiff.ito`](../series-calc/antidiff_ito.md): the integrand as a function.
- [`quadratic_variation`](quadratic_variation.md): the quantity that separates the conventions.
