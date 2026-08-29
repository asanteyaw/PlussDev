<!-- pluss-hand-authored -->

# Series::sign

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sign() const -> Series
```

## What It Does

Returns the elementwise sign summary.

For each element, `sign()` keeps only directional information:

- negative values become `-1`
- zero becomes `0`
- positive values become `1`

This is the complement of [`Series::abs`](abs.md). `abs()` keeps magnitude and
throws direction away; `sign()` keeps direction and throws magnitude away.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, containing only sign-coded
    values.

## Gradient

`sign()` is best taught as a discrete directional summary rather than a smooth
differentiable transform. Its main use is value inspection, rule building, and
piecewise control logic.

## Examples

Example 1. Negative, zero, and positive values collapse to `-1`, `0`, and `1`.

```cpp
auto base = pluss::make_series({
    {-3.0, -2.0, -1.0},
    {0.0, 1.0, 4.0},
});

pluss::series::print(std::cout, base.sign());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -1.0000 -1.0000 -1.0000 ⎤
⎣  0.0000  1.0000  1.0000 ⎦
```

Example 2. Integer inputs produce integer sign-coded outputs.

```cpp
std::cout << pluss::make_series({-9, 0, 11}, pluss::ScalarType::Int64).sign();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 1]
```

Example 3. `sign()` can be a compact way to surface edge behavior, including
special values.

```cpp
std::cout << pluss::make_series({
    -1.0,
    0.0,
    1.0,
    std::numeric_limits<double>::quiet_NaN(),
}).sign();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-1, 0, 1, 0]
```

## Gotchas And Failure Modes

- `sign()` intentionally discards magnitude. After this transform, `1000` and
  `1` are indistinguishable if they had the same sign.
- On the verified `NaN` example above, the output slot is `0`. That is worth
  calling out because users often expect either `nan` propagation or an error.

## Summary

Use `sign()` when you need directional classification, not measurement. It is
especially useful before branching into piecewise logic or when you want to
summarize polarity without preserving scale.

## See Also

- [`Series::abs`](abs.md): magnitude without direction.
- [`Series::neg`](neg.md): sign flip without losing magnitude.
- [`Series::log`](log.md): a domain-sensitive transform where sign often
  determines whether the call is meaningful.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="setxor_along.html">
    <span class="label">Previous</span>
    <span class="title">setxor_along</span>
  </a>
  <a class="next-link" href="sin.html">
    <span class="label">Next</span>
    <span class="title">sin</span>
  </a>
</div>
