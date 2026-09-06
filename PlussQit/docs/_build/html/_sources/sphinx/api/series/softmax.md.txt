<!-- pluss-hand-authored -->

# Series::softmax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::softmax(std::int64_t dim) const -> Series
```

## What It Does

`softmax()` turns a lane of real-valued scores into a probability
distribution over that lane: every output is in `(0, 1)` and the outputs along
`dim` sum to `1`.

```
softmax(x)_i = exp(x_i) / sum_j exp(x_j)
```

The `dim` argument chooses which axis is normalized. Every other axis is left
alone, so on a `[batch, classes]` matrix `softmax(1)` produces one distribution
per row.

Softmax is shift-invariant: adding the same constant to every score in a lane
does not change the result, because the constant cancels between numerator and
denominator. That property is what makes a numerically stable implementation
possible (see the Numerical Stability section below) and it is why only
score *differences* matter.

## Parameters

**dim** (`std::int64_t`)

The axis to normalize over. Negative values count from the end. It must refer
to a real axis; an out-of-range `dim` raises.

There is no whole-series overload — softmax is always taken along one named
axis.

## Returns

`Series`
    A distribution the same shape and dtype as the input. Values along `dim`
    sum to `1`.

## Numerical Stability

A direct `exp(x_i) / sum exp(x_j)` overflows to `inf` as soon as any score
passes about `709` (for `float64`). `softmax()` uses the shift-invariance
above: it subtracts the per-lane maximum before exponentiating,

```
softmax(x)_i = exp(x_i - m) / sum_j exp(x_j - m),   m = max_j x_j
```

so the largest exponent is always `exp(0) = 1` and nothing overflows. The
smallest terms may still underflow to `0` — that is correct behavior, and it
is exactly where [`log_softmax()`](log_softmax.md) earns its place, because
`log(0)` is `-inf` but `log_softmax` stays finite.

## Gradient

`softmax()` is differentiable. Writing `p = softmax(x)` along `dim`, the
Jacobian within a lane is

```
d p_i / d x_j = p_i (delta_ij - p_j)
```

so the backward for an upstream sensitivity `g` is

```
dx_i = p_i (g_i - sum_j p_j g_j)
```

— each input's gradient is its probability times how much its own upstream
signal exceeds the lane's probability-weighted average. A consequence:
`softmax(x).sum(dim)` is the constant `1`, so its gradient is `0` everywhere.

## Examples

Example 1. One distribution per row with `dim = 1`.

```cpp
auto scores = pluss::make_series({
    {1.0, 2.0, 3.0},
    {1.0, 1.0, 1.0},
});

std::cout << scores.softmax(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.090030573, 0.24472847, 0.66524096],
  [0.33333333, 0.33333333, 0.33333333]]
```

Each row sums to `1`. The second row is uniform because its scores are equal.

Example 2. Changing `dim` changes which lane is normalized.

```cpp
auto scores = pluss::make_series({
    {1.0, 2.0, 3.0},
    {1.0, 1.0, 1.0},
});

std::cout << scores.softmax(0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.5, 0.73105858, 0.88079708],
  [0.5, 0.26894142, 0.11920292]]
```

Now each *column* sums to `1`.

Example 3. Negative `dim` counts from the end.

```cpp
auto scores = pluss::make_series({
    {1.0, 2.0, 3.0},
    {1.0, 1.0, 1.0},
});

std::cout << scores.softmax(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.090030573, 0.24472847, 0.66524096],
  [0.33333333, 0.33333333, 0.33333333]]
```

For a rank-2 input, `-1` is the last axis, so this matches `softmax(1)`.

Example 4. Large scores that would overflow a naive `exp` are handled.

```cpp
auto big = pluss::make_series({710.0, 711.0, 712.0});

std::cout << big.softmax(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.090030573, 0.24472847, 0.66524096]
```

`exp(710)` alone overflows `float64`, but the max-shift keeps every exponent at
or below `exp(0)`. The result matches Example 1's first row because the score
differences are the same.

Example 5. The Jacobian in action: gradient of the first probability.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}});
x.tracks_slope_(true);

auto p = x.softmax(1);
(p * pluss::make_series({{1.0, 0.0, 0.0}})).sum().reverse();

std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0.081925069, -0.022033045, -0.059892025]]
```

With `g = [1, 0, 0]` the backward is `p_i (g_i - sum_j p_j g_j) =
p_i (g_i - p_0)`: `p_0(1 - p_0) = 0.0819`, `-p_0 p_1 = -0.0220`,
`-p_0 p_2 = -0.0599`.

Example 6. `softmax(x).sum(dim)` is identically `1`, so its gradient is zero.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}});
x.tracks_slope_(true);

x.softmax(1).sum().reverse();

std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[-1.5025235e-17, -4.0842823e-17, 5.5868057e-17]]
```

Zero up to floating-point rounding.

Example 7. An out-of-range `dim` is rejected.

```cpp
try {
  pluss::make_series({{1.0, 2.0, 3.0}}).softmax(5);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
reduce: dim out of range
```

## Gotchas And Failure Modes

- `dim` is required — there is no whole-series softmax. Pick the axis the
  distribution should live on.
- Only score *differences* matter; a constant offset per lane is invisible.
- The smallest probabilities can underflow to exactly `0`. If you then take a
  log, use [`log_softmax()`](log_softmax.md), which never produces `-inf` for a
  finite input.
- For a classification loss, prefer `log_softmax()` directly over
  `softmax().log()` — it is both more stable and cheaper.

## Summary

`softmax(dim)` normalizes one axis into a probability distribution:

- `exp(x_i) / sum exp(x_j)`, computed with a max-shift so nothing overflows
- shift-invariant along the lane; only score differences matter
- differentiable, Jacobian `p_i (delta_ij - p_j)`
- pair with [`log_softmax()`](log_softmax.md) whenever a logarithm follows

## See Also

- [`Series::log_softmax`](log_softmax.md): `log(softmax(x))` without the
  `-inf`; the right choice for classification losses.
- [`Series::logsumexp`](logsumexp.md): the stable `log sum exp` that softmax's
  denominator is built on.
- [`Series::sigmoid`](sigmoid.md): the two-outcome special case.
- [`Series::amax`](amax.md): the per-lane maximum used for the shift.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="softplus.html">
    <span class="label">Next</span>
    <span class="title">softplus</span>
  </a>
</div>
