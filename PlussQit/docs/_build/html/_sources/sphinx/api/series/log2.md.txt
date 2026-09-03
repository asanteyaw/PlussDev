<!-- pluss-hand-authored -->

# Series::log2

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::log2() const -> Series
Series& Series::log2_()
```

## What It Does

`log2()` is the base-2 logarithm, applied elementwise: `log2(x)` is the power to
which `2` must be raised to get `x`.

It is the inverse of [`exp2()`](exp2.md). Reach for it when the answer is
naturally measured in bits or octaves — information content, the number of
halvings in a binary search, the exponent of the nearest power of two.

Anchor values: `log2(1) = 0`, `log2(2) = 1`, `log2(8) = 3`, `log2(0.5) = -1`.

Domain:

- `x > 0` gives an ordinary result
- `x = 0` gives `-inf`
- `x < 0` gives `NaN`

An integer input is promoted to a floating-point result.

## Parameters

None.

## Returns

`Series` or `Series&`
    The base-2 logarithm elementwise. `log2_()` overwrites the current storage.

## Gradient

`log2()` is differentiable:

```
d/dx log2(x) = 1 / (x * ln 2)
```

That is the natural-log derivative `1/x` scaled by `1 / ln 2` (about `1.4427`).
The gradient is large near `x = 0` and shrinks as `x` grows.

## Examples

Example 1. Powers of two map to integers.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 8.0, 0.5}).log2());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 1.0000, 3.0000, -1.0000]
```

Example 2. The domain edges. `0` gives `-inf`, a negative gives `NaN`.

```cpp
pluss::series::print(std::cout, pluss::make_series({0.0, -1.0}).log2());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-inf, nan]
```

Example 3. The backward multiplier is `1 / (x * ln 2)`.

```cpp
auto x = pluss::make_series({1.0, 2.0, 4.0});
x.tracks_slope_(true);

x.log2().sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.442695, 0.72134752, 0.36067376]
```

Each entry is `1.442695 / x`, since `1 / ln 2 = 1.442695`.

## Gotchas And Failure Modes

- `log2(0)` is `-inf` and `log2(x < 0)` is `NaN`; neither throws.
- The result is floating point even for an integer input.
- For a natural-log quantity use [`log()`](log.md); for base 10 use
  [`log10()`](log10.md). They differ only by a constant scale.

## Summary

`log2(x)` is the base-2 logarithm:

- inverse of [`exp2()`](exp2.md)
- `-inf` at `0`, `NaN` for negatives
- promotes integer input to floating point
- differentiable, backward multiplier `1 / (x * ln 2)`

## See Also

- [`Series::exp2`](exp2.md): the inverse.
- [`Series::log`](log.md): natural logarithm; base-2 is this scaled by
  `1 / ln 2`.
- [`Series::log10`](log10.md): base-10 logarithm.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log1p.html">
    <span class="label">Previous</span>
    <span class="title">log1p</span>
  </a>
  <a class="next-link" href="logaddexp.html">
    <span class="label">Next</span>
    <span class="title">logaddexp</span>
  </a>
</div>
