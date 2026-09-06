<!-- pluss-hand-authored -->

# Series::log_softmax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::log_softmax(std::int64_t dim) const -> Series
```

## What It Does

`log_softmax()` is the logarithm of [`softmax()`](softmax.md), computed as one
fused operation so it never returns `-inf` for a finite input:

```
log_softmax(x)_i = x_i - log( sum_j exp(x_j) )
                 = x_i - logsumexp(x)
```

The `dim` argument chooses which axis is normalized; every other axis is left
alone. On a `[batch, classes]` matrix, `log_softmax(1)` produces one row of log
probabilities per example.

It exists as its own method — rather than leaving you to write
`x.softmax(dim).log()` — for two reasons:

- **Stability.** `softmax()` can legitimately underflow a small probability to
  exactly `0`, and `log(0)` is `-inf`. `log_softmax()` subtracts
  `logsumexp(x)` from the raw scores instead, so every output is a finite real
  number.
- **Cost and gradient quality.** The fused form skips a division and an
  exponential-then-logarithm round trip, and its backward is simpler and better
  conditioned.

This is the function a classification loss should be built on: cross-entropy
for a one-hot target `t` is just `-(t * log_softmax(x)).sum(dim)`.

## Parameters

**dim** (`std::int64_t`)

The axis to normalize over. Negative values count from the end. Must refer to a
real axis; an out-of-range `dim` raises. There is no whole-series overload.

## Returns

`Series`
    Log probabilities, the same shape and dtype as the input. Each value is
    `<= 0`; `exp()` of the result along `dim` sums to `1`.

## Gradient

`log_softmax()` is differentiable. With `p = softmax(x)` and an upstream
sensitivity `g`, the backward along `dim` is

```
dx_i = g_i - p_i * sum_j g_j
```

For the cross-entropy case `L = -(t * log_softmax(x)).sum(dim)` with a
probability vector `t` (sum `1`), this collapses to the well-known

```
dL/dx = softmax(x) - t
```

— the prediction minus the target, with no `softmax` Jacobian to form
explicitly.

## Examples

Example 1. Row-wise log probabilities.

```cpp
auto scores = pluss::make_series({
    {1.0, 2.0, 3.0},
    {1.0, 1.0, 1.0},
});

std::cout << scores.log_softmax(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-2.407606, -1.407606, -0.40760596],
  [-1.0986123, -1.0986123, -1.0986123]]
```

`exp()` of each row sums to `1`; the uniform second row is
`log(1/3) = -1.0986` three times.

Example 2. Where it beats `softmax().log()`.

```cpp
auto z = pluss::make_series({0.0, 1000.0});

std::cout << "log(softmax(z)) = " << z.softmax(0).log();
std::cout << "log_softmax(z)  = " << z.log_softmax(0);
```

### Output

```text
log(softmax(z)) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-inf, 0]
log_softmax(z)  = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1000, 0]
```

`softmax([0, 1000])` underflows the first probability to `0`, so its log is
`-inf`. `log_softmax` keeps the exact value `0 - 1000 = -1000`.

Example 3. Negative `dim`.

```cpp
auto scores = pluss::make_series({
    {1.0, 2.0, 3.0},
    {1.0, 1.0, 1.0},
});

std::cout << scores.log_softmax(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-2.407606, -1.407606, -0.40760596],
  [-1.0986123, -1.0986123, -1.0986123]]
```

Example 4. Cross-entropy gradient is `softmax(x) - target`.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}});
x.tracks_slope_(true);

auto target = pluss::make_series({{0.0, 1.0, 0.0}});   // class 1
auto loss = x.log_softmax(1).mul(target).sum().neg();
loss.reverse();

std::cout << "loss=" << loss.item<double>() << "\n";
std::cout << "dL/dx=" << x.sensitivity();
```

### Output

```text
loss=1.40761
dL/dx=pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0.090030573, -0.75527153, 0.66524096]]
```

The gradient is `softmax(x) - target = [0.0900, 0.2447 - 1, 0.6652]`, and the
loss is `-log_softmax(x)_1 = 1.4076`.

Example 5. An out-of-range `dim` is rejected.

```cpp
try {
  pluss::make_series({{1.0, 2.0, 3.0}}).log_softmax(5);
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

- `dim` is required; there is no whole-series overload.
- Prefer `log_softmax(x)` to `softmax(x).log()` in every case — it is finite
  where the latter is `-inf`, and it is cheaper.
- The output is not a probability distribution; it is the *log* of one.
  `exp(log_softmax(x))` recovers the probabilities.
- Only score differences matter (a constant per-lane offset is invisible),
  exactly as for [`softmax()`](softmax.md).

## Summary

`log_softmax(dim)` is the fused, finite `log(softmax(x))`:

- `x_i - logsumexp(x)` along `dim`
- never `-inf` for finite input; cheaper than `softmax().log()`
- differentiable, backward `g_i - p_i * sum g`; cross-entropy gradient is
  `softmax(x) - target`
- the right building block for a classification loss

## See Also

- [`Series::softmax`](softmax.md): the distribution itself.
- [`Series::logsumexp`](logsumexp.md): the `log sum exp` term subtracted here.
- [`Series::logcumsumexp`](logcumsumexp.md): the running-prefix version of that
  term.
- [`Series::nan_to_num`](nan_to_num.md): a blunt way to patch `-inf` after the
  fact — `log_softmax` avoids needing it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log.html">
    <span class="label">Previous</span>
    <span class="title">log</span>
  </a>
  <a class="next-link" href="log10.html">
    <span class="label">Next</span>
    <span class="title">log10</span>
  </a>
</div>
