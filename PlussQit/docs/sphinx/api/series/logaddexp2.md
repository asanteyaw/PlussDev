<!-- pluss-hand-authored -->

# Series::logaddexp2

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::logaddexp2(const Series& other) const
Series& Series::logaddexp2_(const Series& other)
```

## What It Does

`logaddexp2()` computes `log2(2^a + 2^b)` elementwise — the base-2 counterpart
of [`logaddexp()`](logaddexp.md).

It is the stable evaluation of that expression. The naive
`(a.exp2() + b.exp2()).log2()` overflows as soon as `a` or `b` is a few
thousand, so `logaddexp2()` factors out the larger operand:

```
m = max(a, b)
result = m + log2(1 + 2^(-|a - b|))
       = m + log1p(2^(-|a - b|)) / ln 2
```

Base 2 is the natural choice when your quantities are measured in bits — log2
probabilities, entropy in bits, or anything downstream of a base-2 logarithm.
For everything phrased in nats, use [`logaddexp()`](logaddexp.md) instead.

## Parameters

**other** (`const Series&`)

The second operand `b`. Broadcasts against the receiver.

## Returns

`Series` or `Series&`
    The elementwise stable `log2(2^a + 2^b)`. `logaddexp2_()` writes the result
    back into the receiver. Floating-point inputs only.

## Gradient

`logaddexp2()` is differentiable. The backward is a base-2 softmax weight:

```
d/da logaddexp2(a, b) = 2^a / (2^a + 2^b) = sigmoid((a - b) * ln 2)
d/db logaddexp2(a, b) = sigmoid((b - a) * ln 2)
```

As with [`logaddexp()`](logaddexp.md) the two gradients sum to `1`, and at
`a == b` they split `0.5 / 0.5`.

## Examples

Example 1. `logaddexp2(x, x) = x + 1`, because `2^x + 2^x = 2^(x+1)`.

```cpp
auto a = pluss::make_series({0.0, 1.0, 3.0});
auto b = pluss::make_series({0.0, 2.0, 3.0});

pluss::series::print(std::cout, a.logaddexp2(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 2.5850, 4.0000]
```

Entry 0 is `0 + 1`. Entry 1 is `log2(2 + 4) = log2 6 = 2.5850`. Entry 2 is
`3 + 1`.

Example 2. Stability: the naive base-2 form overflows here, `logaddexp2()` does
not.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1000.0}).logaddexp2(pluss::make_series({1000.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1001.0000]
```

Example 3. The backward is the base-2 softmax weight and sums to `1`.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({2.0, 2.0, 1.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.logaddexp2(b).sum().reverse();

std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.33333333, 0.5, 0.8]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.66666667, 0.5, 0.2]
```

For `a - b = -1`, the weight on `a` is `2^-1 / (2^-1 + 1) = 1/3`. At `a == b`
it is `0.5`. For `a - b = 2`, it is `4 / (4 + 1) = 0.8`.

## Gotchas And Failure Modes

- Prefer `logaddexp2()` over `(a.exp2() + b.exp2()).log2()` whenever the
  operands can be large.
- Floating-point inputs only.
- `+inf` makes the result `+inf`; two `-inf` operands give `-inf`; `NaN`
  propagates.
- Base 2, not base `e`. Mixing this up with [`logaddexp()`](logaddexp.md)
  rescales the result by `ln 2`.

## Summary

`logaddexp2(a, b)` is a stable `log2(2^a + 2^b)`:

- broadcasts like `add()`; floating point only
- built on `m + log1p(2^(-|a - b|)) / ln 2`
- differentiable, gradients `sigmoid((a - b) * ln 2)` and its mirror, summing
  to `1`

## See Also

- [`Series::logaddexp`](logaddexp.md): the natural-log version; use it for
  quantities in nats.
- [`Series::sigmoid`](sigmoid.md): the shape of the backward weight.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logaddexp.html">
    <span class="label">Previous</span>
    <span class="title">logaddexp</span>
  </a>
  <a class="next-link" href="logcumsumexp.html">
    <span class="label">Next</span>
    <span class="title">logcumsumexp</span>
  </a>
</div>
