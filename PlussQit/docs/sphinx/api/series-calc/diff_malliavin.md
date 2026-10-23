<!-- pluss-hand-authored -->

# Series::CalcAccessor::diff.malliavin

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signatures

```cpp
Series Series::CalcAccessor::DiffAccessor::malliavin(
    const std::function<Series(const Series&)>& F, std::int64_t dim = 0) const

Series malliavin_path(
    const std::function<Series(const Series&)>& F, std::int64_t dim = 0) const

Series skorokhod(
    const std::function<Series(const Series&)>& u, double dt, std::int64_t dim = 0) const
```

## What It Computes

Discretize a Wiener path into increments `ΔW_1, …, ΔW_n`. A functional `F` of the
path is a function of those increments, and its **Malliavin derivative** is the
gradient with respect to them:

```text
D_i F = ∂F / ∂(ΔW_i)
```

`malliavin` takes the increments as the receiver and returns that gradient. It is
computed by one reverse pass, however many increments there are. `malliavin_path`
takes the *path* `W` (n + 1 samples starting at 0) as the receiver and
differentiates with respect to its n increments, so `F` sees the path rebuilt from
them.

`skorokhod` computes the **Skorokhod integral** (the divergence, adjoint of the
Malliavin derivative) of a process `u = u(ΔW)` that may look into the future:

```text
δ(u) = Σ u_i ΔW_i − Σ (∂u_i / ∂ΔW_i) Δt
```

For an adapted `u` the second sum vanishes and `δ(u)` is the Itô sum. For an
anticipating `u` it is still defined. The two are linked by the duality `E[F δ(u)]
= E[Σ u_i D_i F Δt]`, the integration by parts that moves a derivative from a
payoff onto a weight.

## Parameters

- The receiver holds the Brownian increments (for `malliavin`, `skorokhod`) or the
  path (for `malliavin_path`) along `dim`. Other axes are independent paths.
- `F` returns one value per remaining index: it reduces the time axis.
- `u` returns a process with the shape of its argument.
- `dt` is the time step (for `skorokhod`).
- `dim` is the time axis.

## Examples

Example 1. `F = W_T²` depends on every increment equally; its derivative is `2 W_T`
on each one.

```cpp
auto dw = pluss::make_series({0.3, -0.5, 0.2, 0.4});

auto d = dw.calc.diff.malliavin([](const pluss::Series& inc) { return inc.sum().pow(2.0); });

std::cout << "derivative " << d << "\n";
std::cout << "2 W_T = " << 2.0 * (0.3 - 0.5 + 0.2 + 0.4) << "\n";
```

### Output

```text
derivative pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.8, 0.8, 0.8, 0.8]
2 W_T = 0.8
```

Example 2. The maximum of the path. Only the increments up to the position of the
maximum move it: here the maximum is the first sample after the start, so only the
first increment has a nonzero derivative.

```cpp
auto dw = pluss::make_series({0.3, -0.5, 0.2, -0.4});
auto path = pluss::Series::zeros({1}, pluss::ScalarType::Float64).unite({dw.cumsum(0)}, 0);

auto d = path.calc.diff.malliavin_path([](const pluss::Series& w) { return w.amax(0); });

std::cout << "path       " << path << "\n";
std::cout << "derivative " << d << "\n";
```

### Output

```text
path       pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0.3, -0.2, 0, -0.4]
derivative pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0, 0, 0]
```

Example 3. For an adapted integrand the Skorokhod integral is the Itô sum. Take `u_i
= W` at the left end of step `i`.

```cpp
auto dw = pluss::make_series({0.3, -0.5, 0.2, 0.4});
auto adapted = [](const pluss::Series& inc) { return inc.cumsum(0).sub(inc); };

auto delta = dw.calc.diff.skorokhod(adapted, 0.25);
auto ito_sum = adapted(dw).mul(dw).sum();

std::cout << "skorokhod " << delta << "\n";
std::cout << "ito sum   " << ito_sum << "\n";
```

### Output

```text
skorokhod pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.19
ito sum   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.19
```

Example 4. An anticipating integrand. With `u_i = W_T` for every `i`, the Itô sum is
not defined, but `δ(u) = W_T² − T`.

```cpp
auto dw = pluss::make_series({0.3, -0.5, 0.2, 0.4});
const double T = 1.0;
auto anticipating = [](const pluss::Series& inc) { return inc.sum().expand({inc.size(0)}); };

auto delta = dw.calc.diff.skorokhod(anticipating, T / 4.0);

const double wT = 0.3 - 0.5 + 0.2 + 0.4;
std::cout << "skorokhod   " << delta << "\n";
std::cout << "W_T^2 - T = " << wT * wT - T << "\n";
```

### Output

```text
skorokhod   pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-0.84
W_T^2 - T = -0.84
```

Example 5. Many paths at once: time on axis 0, paths on axis 1. The derivative has
the shape of the increments.

```cpp
auto dw = pluss::series::randn({6, 3}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 7);

auto d = dw.calc.diff.malliavin([](const pluss::Series& inc) { return inc.sum(0).pow(2.0); });

std::cout << "shape of derivative [" << d.size(0) << ", " << d.size(1) << "]\n";
std::cout << "equals 2 W_T on each increment: "
          << d.sub(dw.sum(0).mul(2.0).unsqueeze(0).expand({6, 3})).abs().amax().item<double>() << "\n";
```

### Output

```text
shape of derivative [6, 3]
equals 2 W_T on each increment: 0
```

## Gotchas And Failure Modes

- `skorokhod` costs one reverse pass per increment. Use it for the grid sizes of a
  test or a sensitivity, not for millions of steps.
- The Malliavin derivative here is the *discrete* derivative of the discretized
  functional. It converges to the continuous one as the grid is refined; on a coarse
  grid, compare against a known case before relying on it.
- `F` must be differentiable in the increments. A functional with an indicator
  (a digital payoff) has derivative zero almost everywhere; that is the case for
  which the Malliavin weight is used instead of the pathwise derivative.

## Summary

`diff.malliavin` differentiates a functional of a Brownian path with respect to its
increments, and `diff.skorokhod` integrates processes that look ahead.

## See Also

- [`diff.ito`](diff_ito.md) and [`antidiff.ito`](antidiff_ito.md): the adapted case.
