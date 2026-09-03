<!-- pluss-hand-authored -->

# Series::xlogy

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::xlogy(const Series& other) const -> Series
Series& Series::xlogy_(const Series& other)
```

## What It Does

`xlogy()` computes `a * log(b)` elementwise, with one deliberate exception: when
`a` is `0` the result is `0`, even if `log(b)` is `-inf` or undefined.

The plain product `a * b.log()` gets this case wrong. At `a = 0, b = 0` it
evaluates `0 * log(0) = 0 * (-inf) = NaN`. But the limit of `a * log(b)` as
`a -> 0` is `0`, and that is the value nearly every application wants.

The name is read "x log y". It shows up wherever a weighted log appears and
some weights are exactly zero:

- entropy, `-sum p * log(p)`, where some `p` are `0`
- cross-entropy and KL divergence between distributions with zeros
- log-likelihoods of count data where some counts are `0`

`xlogy()` lets you write those sums directly without masking the zero terms by
hand.

## Parameters

**other** (`const Series&`)

The value `b` whose logarithm is taken. Broadcasts against the receiver.

## Returns

`Series` or `Series&`
    `a * log(b)`, except `0` wherever `a == 0`. A `NaN` in `b` still
    propagates. `xlogy_()` writes the result back into the receiver.

## Gradient

`xlogy()` is differentiable:

```
d/da xlogy(a, b) = log(b)
d/db xlogy(a, b) = a / b
```

Because the `a == 0` result is pinned to the constant `0`, the gradient through
those positions is `0` for both operands — it does not carry the `log(b)` term
there. Everywhere else the rule is the ordinary product rule.

## Examples

Example 1. A zero weight zeroes the term even when `log(b)` is `-inf`.

```cpp
auto a = pluss::make_series({0.0, 2.0, 3.0});
auto b = pluss::make_series({0.0, 4.0, 8.0});

pluss::series::print(std::cout, a.xlogy(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 2.7726, 6.2383]
```

The first term is `0` (not `NaN`). The others are `2 * log 4` and `3 * log 8`.

Example 2. A non-zero weight against `b = 0` is still `-inf`; only `a == 0` is
special-cased.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2.0}).xlogy(pluss::make_series({0.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-inf]
```

Example 3. A `NaN` in `b` propagates regardless of `a`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0}).xlogy(pluss::make_series({std::nan("")})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[nan]
```

Example 4. The gradient is `(log(b), a / b)` on the non-zeroed positions.

```cpp
auto a = pluss::make_series({2.0, 3.0});
auto b = pluss::make_series({4.0, 5.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.xlogy(b).sum().reverse();

std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
da=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.3862944, 1.6094379]
db=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 0.6]
```

`da` is `log 4` and `log 5`; `db` is `2/4` and `3/5`.

## Gotchas And Failure Modes

- Only `a == 0` is special-cased. `a` slightly nonzero with `b == 0` still
  gives `-inf`.
- A `NaN` in `b` propagates even when `a == 0`.
- Floating-point inputs.
- `xlogy()` is not symmetric: `a.xlogy(b)` is `a * log(b)`, not `b * log(a)`.

## Summary

`xlogy(a, b)` is `a * log(b)` with the convention `0 * log(0) = 0`:

- broadcasts like `add()`
- fixes the `NaN` that the plain product produces at `a = b = 0`
- differentiable, gradient `(log(b), a / b)`, and `0` through the pinned
  positions

## See Also

- [`Series::log`](log.md): the logarithm `xlogy()` wraps.
- [`Series::mul`](mul.md): the plain product, which does not special-case the
  zero weight.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="where_.html">
    <span class="label">Previous</span>
    <span class="title">where_</span>
  </a>
  <a class="next-link" href="../../series_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Series API Reference</span>
  </a>
</div>
