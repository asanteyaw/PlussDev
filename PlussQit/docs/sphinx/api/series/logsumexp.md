<!-- pluss-hand-authored -->

# Series::logsumexp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::logsumexp() const
Series Series::logsumexp(std::int64_t dim) const
Series Series::logsumexp(std::int64_t dim, bool keepdim) const
```

## What It Does

`logsumexp()` reduces an axis by `log(sum(exp(x)))` — the log of the total of
the exponentials.

Naively that reads "exponentiate, add up, take the log," and naively it is
unusable: for `x` around `1000`, `exp(x)` is already past the top of `float64`
and the whole thing collapses to `inf`. `logsumexp()` is the stable version.
It shifts by the maximum first:

```
m = max(x)                       (along the reduced axis)
logsumexp(x) = m + log(sum(exp(x - m)))
```

Every shifted exponent `x - m` is `<= 0`, so `exp` of it lies in `(0, 1]` and
cannot overflow, while the largest term contributes exactly `1`.

This is the normalizing constant of a softmax. `softmax(x) = exp(x -
logsumexp(x))`, and a cross-entropy loss is `logsumexp(logits) - logits[target]`.
Anywhere probabilities are carried as unnormalized log-scores, `logsumexp()` is
how you total them without leaving log-space.

The no-argument form reduces the whole series to a scalar. The `dim` forms
reduce one axis; `keepdim` controls whether that axis is kept as a length-1
dimension.

## Parameters

**dim** (`std::int64_t`, optional)

The axis to reduce. Negative values count from the back. Omitted: reduce
everything to a scalar.

**keepdim** (`bool`, default `false`)

Keep the reduced axis as size `1` instead of removing it. Useful when the
result is broadcast back against the input, as in `x - x.logsumexp(dim, true)`
to build a log-softmax.

## Returns

`Series`
    The reduced result. A scalar for the no-argument form; otherwise the input
    shape with `dim` removed (or kept as `1` when `keepdim` is set). Floating
    point.

## Gradient

`logsumexp()` is differentiable, and its backward is the softmax of the input:

```
d/dx_k logsumexp(x) = exp(x_k) / sum_j exp(x_j) = softmax(x)_k
```

The gradients along the reduced axis are non-negative and sum to `1` — the
upstream sensitivity is distributed across the inputs in proportion to how much
each contributed to the total.

## Examples

Example 1. A whole-series reduction to a scalar.

```cpp
pluss::series::print(std::cout, pluss::make_series({1.0, 2.0, 3.0}).logsumexp(0));
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.4076
```

`log(e^1 + e^2 + e^3) = 3.4076`.

Example 2. Reducing one axis of a matrix.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

std::cout << "dim 0: ";
pluss::series::print(std::cout, m.logsumexp(0));
std::cout << "\ndim 1: ";
pluss::series::print(std::cout, m.logsumexp(1));
```

### Output

```text
dim 0: Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3.1269, 4.1269]
dim 1: Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.3133, 4.3133]
```

Example 3. `keepdim` keeps the reduced axis as size `1` for broadcasting back.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

std::cout << m.logsumexp(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[2.3132617],
  [4.3132617]]
```

Example 4. The stability that motivates the method. Every element is `1000`;
the naive `x.exp().sum().log()` overflows, `logsumexp()` returns the right
answer.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1000.0, 1000.0, 1000.0}).logsumexp(0));
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1001.0986
```

`1000 + log(3) = 1001.0986`.

Example 5. `-inf` entries drop out cleanly, as zero-probability terms.

```cpp
double ninf = -std::numeric_limits<double>::infinity();

pluss::series::print(
    std::cout,
    pluss::make_series({ninf, ninf, 2.0}).logsumexp(0));
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.0000
```

Example 6. The backward pass is the softmax of the input.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

x.logsumexp(0).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.090030573, 0.24472847, 0.66524096]
```

Those are `softmax([1, 2, 3])`, and they sum to `1`.

## Gotchas And Failure Modes

- Reach for `logsumexp()` rather than `x.exp().sum().log()` whenever the values
  can be large. The naive form overflows to `inf`.
- The no-argument form reduces everything; pass `dim` to reduce a single axis.
- All-`-inf` input reduces to `-inf`; a `NaN` anywhere in the reduced axis
  propagates.
- Floating-point input.

## Summary

`logsumexp(x)` is a stable `log(sum(exp(x)))` reduction:

- shifts by the max so nothing overflows
- `dim` / `keepdim` behave like the other reductions
- differentiable, backward is `softmax(x)` along the reduced axis

## See Also

- [`Series::logcumsumexp`](logcumsumexp.md): the running (prefix) version along
  an axis.
- [`Series::logaddexp`](logaddexp.md): the two-argument, broadcasting form.
- [`Series::sum`](sum.md): the plain additive reduction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logit.html">
    <span class="label">Previous</span>
    <span class="title">logit</span>
  </a>
  <a class="next-link" href="lt.html">
    <span class="label">Next</span>
    <span class="title">lt</span>
  </a>
</div>
