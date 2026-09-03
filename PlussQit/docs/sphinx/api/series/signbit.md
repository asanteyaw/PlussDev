<!-- pluss-hand-authored -->

# Series::signbit

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::signbit() const -> Series
```

## What It Does

`signbit()` returns a boolean mask that is `true` wherever the value's IEEE
sign bit is set.

For ordinary numbers that is the same as `x < 0`, but `signbit()` is stricter
in exactly the two places where the sign bit and "is negative" disagree:

- `signbit(-0.0)` is `true`, while `-0.0 < 0.0` is `false`
- `signbit(-NaN)` is `true`, while any comparison with `NaN` is `false`

Those cases matter when you are inspecting the result of a computation that can
legitimately produce a negative zero or a signed `NaN` — restoring a sign with
[`copysign()`](copysign.md), branchless sign handling, or checking which way a
value approached zero.

For integer and boolean input, which have no signed zero, the result is exactly
`x < 0`.

## Parameters

None.

## Returns

`Series`
    A boolean `Series` with the same shape as `self`, `true` where the sign bit
    is set.

## Gradient

`signbit()` is not differentiable. It is a bit-level classification.

## Examples

Example 1. Ordinary values behave like `x < 0`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2.0, 0.0, 3.0}).signbit());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
```

Example 2. A negative zero has its sign bit set, so `signbit()` reports `true`
where `< 0` would not. Here `-0.0` is produced by negating a `0.0`.

```cpp
auto negative_zero = pluss::make_series({0.0}).neg();

pluss::series::print(std::cout, negative_zero.signbit());
```

### Output

```text
Series(shape=[1], strides=[1], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=[true]
```

Example 3. A signed `NaN` also has its sign bit set; an unsigned `NaN` does
not.

```cpp
double neg_nan = -std::nan("");
double pos_nan = std::nan("");

pluss::series::print(
    std::cout,
    pluss::make_series({neg_nan, pos_nan}).signbit());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

Example 4. Integer input: no signed zero, so the result is just `x < 0`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-2, 0, 3}, pluss::ScalarType::Int64).signbit());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
```

## Gotchas And Failure Modes

- `signbit()` is not `x < 0`. They differ at `-0.0` and at a signed `NaN`. If
  you actually want the numeric comparison, use [`lt()`](lt.md).
- The output dtype is always `bool`.
- Not differentiable.

## Summary

`signbit(x)` is the IEEE sign bit as a boolean mask:

- `true` for negatives, `-0.0`, and `-NaN`
- equals `x < 0` for integer and boolean input
- not differentiable

## See Also

- [`Series::sign`](sign.md): the `-1 / 0 / +1` sign value rather than a bit.
- [`Series::copysign`](copysign.md): apply a sign taken from another series.
- [`Series::lt`](lt.md): the plain `x < 0` comparison `signbit()` is often
  confused with.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sign.html">
    <span class="label">Previous</span>
    <span class="title">sign</span>
  </a>
  <a class="next-link" href="silu.html">
    <span class="label">Next</span>
    <span class="title">silu</span>
  </a>
</div>
