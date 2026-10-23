<!-- pluss-hand-authored -->

# Series::AntidiffAccessor::quadratic_variation

[Back to Series::AntidiffAccessor API Reference](../../series_antidiff_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesAntidiffProxy::quadratic_variation(std::int64_t dim = -1) const
Series SeriesAntidiffProxy::quadratic_variation_cumulative(std::int64_t dim = -1) const
Series SeriesAntidiffProxy::covariation(std::int64_t dim = -1) const
Series SeriesAntidiffProxy::covariation_cumulative(std::int64_t dim = -1) const
```

## What It Does

The quadratic variation of a sampled path is the sum of its squared increments:

```text
[w] = Σ (w_{i+1} − w_i)²
```

For a standard Brownian path it equals the elapsed time however fine the grid, and
that non-vanishing value is what separates Itô from Stratonovich integrals. A smooth
path has quadratic variation near zero; a path with jumps has the squares of its
jumps added.

The covariation of two paths is `Σ Δx_i Δy_i`. It obeys the polarization identity `[x
+ y] = [x] + [y] + 2 [x, y]`.

`w.antidiff().quadratic_variation()` needs no peer. `x.antidiff(y).covariation()`
uses the peer `y`. The `_cumulative` forms return the running sum: the length of the
path, zero at the first sample.

## Parameters

- The receiver is the sampled path (`n` samples along `dim`).
- The peer, for `covariation`, is the second path of the same length.
- `dim` is the time axis. The default is the last axis.

## Examples

Example 1. A Brownian path has quadratic variation near its elapsed time; a straight
line has almost none.

```cpp
const std::int64_t n = 2000;
auto dw = pluss::series::randn({n}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 3).mul(std::sqrt(1.0 / n));
auto brownian = pluss::Series::zeros({1}, pluss::ScalarType::Float64).unite({dw.cumsum(0)}, 0);
auto line = pluss::Series::linspace(0.0, 1.0, n + 1, pluss::ScalarType::Float64);

std::cout << "brownian " << brownian.antidiff().quadratic_variation() << "\n";
std::cout << "line     " << line.antidiff().quadratic_variation() << "\n";
```

### Output

```text
brownian pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.97883693
line     pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.0005
```

Example 2. The running quadratic variation, and the exact identity between the two
conventions: Stratonovich minus Itô equals half the quadratic variation.

```cpp
auto w = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});

std::cout << "running [w] " << w.antidiff().quadratic_variation_cumulative() << "\n";
std::cout << "strat - ito " << w.antidiff(w).strat().sub(w.antidiff(w).ito()) << "\n";
std::cout << "[w]/2       " << w.antidiff().quadratic_variation().mul(0.5) << "\n";
```

### Output

```text
running [w] pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0.09, 0.34, 0.38, 0.54]
strat - ito pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.27
[w]/2       pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.27
```

Example 3. Covariation and the polarization identity.

```cpp
auto x = pluss::make_series({0.0, 0.3, -0.2, 0.0, 0.4});
auto y = pluss::make_series({0.0, -0.1, 0.2, 0.5, 0.3});

auto cov = x.antidiff(y).covariation();
auto sum_qv = x.add(y).antidiff().quadratic_variation();
auto rhs = x.antidiff().quadratic_variation().add(y.antidiff().quadratic_variation()).add(cov.mul(2.0));

std::cout << "[x, y]            " << cov << "\n";
std::cout << "[x + y]           " << sum_qv << "\n";
std::cout << "[x] + [y] + 2[x,y] " << rhs << "\n";
std::cout << "running [x, y]    " << x.antidiff(y).covariation_cumulative() << "\n";
```

### Output

```text
[x, y]            pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.2
[x + y]           pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.37
[x] + [y] + 2[x,y] pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.37
running [x, y]    pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, -0.03, -0.18, -0.12, -0.2]
```

Example 4. Batches: time on axis 0, one value per path.

```cpp
auto w = pluss::make_series({{0.0, 0.0}, {0.3, -0.1}, {-0.2, 0.2}, {0.0, 0.5}, {0.4, 0.3}});  // [5, 2]

std::cout << "per path " << w.antidiff().quadratic_variation(0) << "\n";
```

### Output

```text
per path pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.54, 0.23]
```

## Gotchas And Failure Modes

- The result depends on the sampling. A path sampled more coarsely has a smaller
  sum of squares; only on a fine grid does a Brownian path approach the elapsed time.
- `covariation` without a peer throws.

## Summary

`quadratic_variation` and `covariation` are the sums of squared and cross increments
of sampled paths; they explain the gap between the Itô and Stratonovich integrals.

## See Also

- [`ito`](stochastic_integrals.md): the sampled stochastic integrals.
- [`calc.antidiff.ito`](../series-calc/antidiff_ito.md): the same, with a function.
