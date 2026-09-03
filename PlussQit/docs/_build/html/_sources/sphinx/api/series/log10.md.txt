<!-- pluss-hand-authored -->

# Series::log10

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::log10() const -> Series
Series& Series::log10_()
```

## What It Does

`log10()` is the base-10 logarithm, applied elementwise: `log10(x)` is the
power to which `10` must be raised to get `x`.

Base 10 is the readable choice when the answer is an order of magnitude — a
decibel level, a pH, the number of digits in an integer, an axis tick on a
log-scaled plot.

Anchor values: `log10(1) = 0`, `log10(10) = 1`, `log10(100) = 2`,
`log10(0.1) = -1`.

Domain:

- `x > 0` gives an ordinary result
- `x = 0` gives `-inf`
- `x < 0` gives `NaN`

An integer input is promoted to a floating-point result.

## Parameters

None.

## Returns

`Series` or `Series&`
    The base-10 logarithm elementwise. `log10_()` overwrites the current
    storage.

## Gradient

`log10()` is differentiable:

```
d/dx log10(x) = 1 / (x * ln 10)
```

That is the natural-log derivative `1/x` scaled by `1 / ln 10` (about
`0.4343`).

## Examples

Example 1. Powers of ten map to integers.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 10.0, 100.0, 0.1}).log10());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 1.0000, 2.0000, -1.0000]
```

Example 2. The backward multiplier is `1 / (x * ln 10)`.

```cpp
auto x = pluss::make_series({1.0, 10.0, 100.0});
x.tracks_slope_(true);

x.log10().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.43429448, 0.043429448, 0.0043429448]
```

Each entry is `0.43429448 / x`, since `1 / ln 10 = 0.43429448`.

## Gotchas And Failure Modes

- `log10(0)` is `-inf` and `log10(x < 0)` is `NaN`; neither throws.
- The result is floating point even for an integer input.
- For a natural-log quantity use [`log()`](log.md); for base 2 use
  [`log2()`](log2.md).

## Summary

`log10(x)` is the base-10 logarithm:

- `-inf` at `0`, `NaN` for negatives
- promotes integer input to floating point
- differentiable, backward multiplier `1 / (x * ln 10)`

## See Also

- [`Series::log`](log.md): natural logarithm; base-10 is this scaled by
  `1 / ln 10`.
- [`Series::log2`](log2.md): base-2 logarithm.
- [`Series::exp`](exp.md): to go the other way, since `10^x = exp(x * ln 10)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log.html">
    <span class="label">Previous</span>
    <span class="title">log</span>
  </a>
  <a class="next-link" href="log1p.html">
    <span class="label">Next</span>
    <span class="title">log1p</span>
  </a>
</div>
