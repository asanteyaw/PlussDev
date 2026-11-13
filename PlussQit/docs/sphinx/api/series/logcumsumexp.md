<!-- pluss-hand-authored -->

# Series::logcumsumexp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::logcumsumexp() const
Series Series::logcumsumexp(std::int64_t dim) const
```

## What It Does

`logcumsumexp()` is the running (prefix) version of
[`logsumexp()`](logsumexp.md). Along the chosen axis, position `t` holds

```
logcumsumexp(x)[t] = log( sum_{j <= t} exp(x[j]) )
```

So it does not reduce the axis away — it keeps the full length and replaces
each position with the log-sum-exp of everything up to and including that
position. For a row `[1, 2, 3]` the result is
`[1, log(e^1 + e^2), log(e^1 + e^2 + e^3)]`.

It is the log-space analogue of [`cumsum()`](cumsum.md): the same prefix-scan
shape, but adding probabilities that are carried as log-scores. It shows up in
the forward pass of sequence models — CTC, the forward algorithm for HMMs,
autoregressive normalizers — anywhere a running total of exponentials would
otherwise overflow.

Like `logsumexp()`, it is evaluated with a max-shift so `exp()` cannot
overflow:

```
M = max(x)                       (along the scanned axis)
logcumsumexp(x) = M + log(cumsum(exp(x - M)))
```

The default axis is the last one.

## Parameters

**dim** (`std::int64_t`, optional)

The axis to scan. Negative values count from the back. Omitted: the last axis.

## Returns

`Series`
    The same shape as the input, each position replaced by the prefix
    log-sum-exp along `dim`. Floating point.

## Gradient

`logcumsumexp()` is differentiable. Its backward is a cumulative softmax: the
sensitivity of output `t` is spread over inputs `0..t` in proportion to
`exp(x[j])`, so an early input feeds many outputs and a late input feeds few.

## Examples

Example 1. A prefix scan along a length-3 row.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0}).logcumsumexp(0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 2.3133, 3.4076]
```

The first entry is just `x[0]`, and the last matches
`logsumexp([1, 2, 3]) = 3.4076`.

Example 2. The max-shift keeps it stable where the naive
`x.exp().cumsum(dim).log()` would overflow. Every element is `1000`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1000.0, 1000.0, 1000.0}).logcumsumexp(0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1000.0000, 1000.6931, 1001.0986]
```

The differences are `log(1)`, `log(2)`, `log(3)`.

Example 3. `-inf` prefixes stay `-inf` (a zero-probability prefix) without
`NaN`.

```cpp
double ninf = -std::numeric_limits<double>::infinity();

pluss::series::print(
    std::cout,
    pluss::make_series({ninf, ninf, 2.0}).logcumsumexp(0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-inf, -inf, 2.0000]
```

Example 4. Scanning one axis of a matrix.

```cpp
auto m = pluss::make_series({
    {1.0, 2.0, 3.0},
    {3.0, 2.0, 1.0},
});

std::cout << m.logcumsumexp(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2.3132617, 3.407606],
  [3, 3.3132617, 3.407606]]
```

Each row is scanned independently.

Example 5. The backward pass is a cumulative softmax.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

x.logcumsumexp(0).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.358972, 0.97578705, 0.66524096]
```

Position `0` contributes to all three prefix sums, so it accumulates the most;
position `2` contributes only to the last, so it accumulates `softmax([1, 2,
3])[2] = 0.665`.

## Gotchas And Failure Modes

- `logcumsumexp()` is a scan, not a reduction: the output keeps the full axis
  length.
- Use it rather than `x.exp().cumsum(dim).log()`, which overflows for large
  values.
- An all-`-inf` prefix stays `-inf`; a `NaN` propagates forward through the
  scan.
- Floating-point input.

## Summary

`logcumsumexp(x)` is the prefix `log(sum(exp(...)))` along an axis:

- shape-preserving scan, default axis is the last
- max-shifted so `exp()` cannot overflow
- differentiable, backward is a cumulative softmax

## See Also

- [`Series::logsumexp`](logsumexp.md): the full reduction this scans toward.
- [`Series::cumsum`](cumsum.md): the plain additive prefix scan.
- [`Series::logaddexp`](logaddexp.md): the two-argument building block.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logaddexp2.html">
    <span class="label">Previous</span>
    <span class="title">logaddexp2</span>
  </a>
  <a class="next-link" href="logical_and.html">
    <span class="label">Next</span>
    <span class="title">logical_and</span>
  </a>
</div>
