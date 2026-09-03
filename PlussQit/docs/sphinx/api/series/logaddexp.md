<!-- pluss-hand-authored -->

# Series::logaddexp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::logaddexp(const Series& other) const -> Series
Series& Series::logaddexp_(const Series& other)
```

## What It Does

`logaddexp()` computes `log(exp(a) + exp(b))` elementwise, where `a` is the
receiver and `b` is `other`.

Written that way it looks trivial, but the naive spelling is a trap. If `a` is
`1000`, then `exp(a)` is about `2e434`, which overflows `float64` to `inf`, and
the whole expression collapses to `inf` even though the true answer is close to
`1000`.

`logaddexp()` is the numerically stable version. It factors out the larger
operand:

```
m = max(a, b)
result = m + log(exp(a - m) + exp(b - m))
       = m + log1p(exp(-|a - b|))
```

Now both shifted exponents are `<= 0`, so `exp` of them is in `(0, 1]` and
cannot overflow. The `log1p` form (`log(1 + x)` computed accurately for small
`x`) also keeps precision when `a` and `b` are far apart.

This operation is the heart of working in log-space: it is how you add two
probabilities that are stored as log-probabilities without leaving log-space.
`logaddexp()` is the two-argument, broadcasting form; a full log-space sum over
an axis is a dedicated `logsumexp` reduction.

## Parameters

**other** (`const Series&`)

The second operand `b`. Broadcasts against the receiver.

## Returns

`Series` or `Series&`
    The elementwise stable `log(exp(a) + exp(b))`. `logaddexp_()` writes the
    result back into the receiver. Floating-point inputs only.

## Gradient

`logaddexp()` is differentiable, with a backward that is a soft indicator of
which operand dominated:

```
d/da logaddexp(a, b) = sigmoid(a - b) = exp(a) / (exp(a) + exp(b))
d/db logaddexp(a, b) = sigmoid(b - a)
```

The two gradients always sum to `1`. When `a` far exceeds `b`, essentially all
of the upstream sensitivity flows to `a`; at `a == b` it splits exactly
`0.5 / 0.5`.

## Examples

Example 1. `logaddexp(x, x) = x + log(2)` for every element.

```cpp
auto a = pluss::make_series({0.0, 1.0, 2.0});
auto b = pluss::make_series({0.0, 1.0, 3.0});

pluss::series::print(std::cout, a.logaddexp(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.6931, 1.6931, 3.3133]
```

The first two entries are `0 + log 2` and `1 + log 2`. The third is
`3 + log1p(exp(-1)) = 3.3133`.

Example 2. The stability that motivates the method. The naive
`(a.exp() + b.exp()).log()` overflows here; `logaddexp()` returns the right
answer.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1000.0}).logaddexp(pluss::make_series({1000.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1000.6931]
```

Example 3. `-inf` behaves as "log of zero probability". Two `-inf` operands
combine to `-inf` with no `NaN`.

```cpp
double ninf = -std::numeric_limits<double>::infinity();

pluss::series::print(
    std::cout,
    pluss::make_series({ninf}).logaddexp(pluss::make_series({ninf})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-inf]
```

Example 4. The backward is `sigmoid(a - b)` and `sigmoid(b - a)`, which sum to
`1`. Note the exact `0.5 / 0.5` split at the tie.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({2.0, 2.0, 1.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.logaddexp(b).sum().reverse();

std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.26894142, 0.5, 0.88079708]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.73105858, 0.5, 0.11920292]
```

## Gotchas And Failure Modes

- Use `logaddexp()` rather than `(a.exp() + b.exp()).log()` whenever the
  operands can be large. The naive form overflows and returns `inf`.
- Floating-point inputs only; integer series are not promoted.
- `+inf` in either operand makes the result `+inf`; two `-inf` operands give
  `-inf`; a `NaN` operand propagates.
- For a log-space sum over a whole axis, reach for a reduction rather than
  chaining `logaddexp()` calls.

## Summary

`logaddexp(a, b)` is a stable `log(exp(a) + exp(b))`:

- broadcasts like `add()`; floating point only
- built on the `m + log1p(exp(-|a - b|))` shift so nothing overflows
- differentiable, gradients `sigmoid(a - b)` and `sigmoid(b - a)` that sum to
  `1`

## See Also

- [`Series::logaddexp2`](logaddexp2.md): the base-2 version.
- [`Series::sigmoid`](sigmoid.md): the function that the backward rule is.
- [`Series::exp`](exp.md) and [`Series::log1p`](log1p.md): the stable pieces
  `logaddexp()` is assembled from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log2.html">
    <span class="label">Previous</span>
    <span class="title">log2</span>
  </a>
  <a class="next-link" href="logaddexp2.html">
    <span class="label">Next</span>
    <span class="title">logaddexp2</span>
  </a>
</div>
