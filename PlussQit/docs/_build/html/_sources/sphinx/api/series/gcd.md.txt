<!-- pluss-hand-authored -->

# Series::gcd

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::gcd(const Series& other) const
Series& Series::gcd_(const Series& other)
Series& Series::gcd_(const Series& rhs) const
```

## What It Does

`gcd()` compares two integer series element by element and returns the greatest
common divisor of every aligned pair. It is a **pairwise transform**, not a
reduction: a six-element receiver and a broadcastable three-element operand
produce six answers rather than one divisor shared by the whole receiver.

For a pair `(a, b)`, the result is the largest non-negative integer that divides
both values without a remainder. This makes `gcd()` useful wherever integers
encode compatible units: reducing fractions, finding an exact block size shared
by several measurements, or normalizing integer ratios before comparison.

Conventions, matching `std::gcd`:

- the result is always `>= 0`, regardless of the signs of the inputs
- `gcd(x, 0) == |x|` and `gcd(0, 0) == 0`

This is an integer-only operation. A floating-point or complex input raises
rather than silently truncating.

## Parameters

**other** (`const Series&`): the second integer operand. Its shape is broadcast
against the receiver using the ordinary trailing-dimension rules. The two
integer dtypes are promoted before the result is returned.

## Returns

`Series` or `Series&`
    A new series containing the non-negative pairwise divisors. Its shape is the
    broadcast shape and its dtype is the promoted integer dtype. `gcd_()` writes
    that result into the receiver without replacing its storage; consequently,
    the broadcast result must fit the receiver's existing shape and dtype.

Raises if either operand is a floating-point or complex series.

## Gradient

`gcd()` is a discrete integer operation and is **not differentiable**.

## In-Place Form

`gcd_()` is the in-place sibling of [`gcd()`](gcd.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `gcd_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series(std::vector<std::int64_t>{12, 18}, {2});
auto y = pluss::make_series(std::vector<std::int64_t>{8, 24}, {2});

x.gcd_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
```

## Examples

Example 1. Signs are dropped; `gcd(x, 0)` is `|x|`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({12, 18, 0, -15}).gcd(pluss::make_series({8, 24, 5, 9})));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[4, 6, 5, 3]
```

`gcd(0, 5) = 5`, and `gcd(-15, 9) = 3` — the negative sign is discarded.

Example 2. Broadcasting a row across a matrix.

```cpp
auto measurements = pluss::make_series({{-12, 18, 0}, {20, -27, 14}});
auto block_sizes = pluss::make_series({{8, 6, 5}});

pluss::series::print(std::cout, measurements.gcd(block_sizes));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 4 6 5 ⎤
⎣ 4 3 1 ⎦
```

The row-shaped `block_sizes` is reused for each row of `measurements`. Notice
that signs never leak into the result and that `gcd(0, 5)` contributes `5`.
This shape-preserving, pairwise form is convenient when each column represents
a different unit or sampling interval.

Example 3. A floating-point input raises.

```cpp
try {
  std::cout << pluss::make_series({12.0}).gcd(pluss::make_series({8.0}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
gcd: integer inputs required
```

## Gotchas And Failure Modes

- Integer inputs only. Convert a float series with `to(ScalarType::Int64)`
  first if truncation is what you actually want.
- `gcd()` may expand either operand through broadcasting. `gcd_()` cannot grow
  or retype its receiver, so use the out-of-place form when the result shape or
  promoted dtype differs.
- The result is never negative, even when both inputs are.
- `gcd(0, 0)` is `0`, not an error.
- Not differentiable.

## Summary

`gcd(a, b)` is the elementwise greatest common divisor:

- non-negative result; `gcd(x, 0) = |x|`, `gcd(0, 0) = 0`
- integer inputs only — a float operand raises
- not differentiable
- `gcd_()` is the in-place form

## See Also

- [`Series::lcm`](lcm.md): the least common multiple, `|a * b| / gcd(a, b)`.
- [`Series::factor`](factor.md): the prime factorization the GCD can be read
  off from.
- [`Series::remainder`](remainder.md): the modulo the Euclidean algorithm
  iterates.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gather.html">
    <span class="label">Previous</span>
    <span class="title">gather</span>
  </a>
  <a class="next-link" href="ge.html">
    <span class="label">Next</span>
    <span class="title">ge</span>
  </a>
</div>
