<!-- pluss-hand-authored -->

# Series::trunc

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::trunc() const -> Series
```

## What It Does

`trunc()` removes the fractional part of each element by rounding toward zero.

This is the key distinction from the other rounding surfaces:

- positive values shrink downward toward zero
- negative values rise upward toward zero

So `trunc()` behaves like "keep the integer part with the original sign."

## Returns

`Series`
    A new `Series` with fractional parts discarded toward zero.

## Gradient

Away from discontinuity points, `trunc()` is locally constant, so reverse-mode
sensitivity is `0` through the rounded output in the ordinary case.

## Examples

Example 1. Negative and positive values both move toward zero.

```cpp
pluss::series::print(std::cout, pluss::make_series({-1.9, -1.1, 0.0, 1.1, 1.9}).trunc());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1.0000, -1.0000, 0.0000, 1.0000, 1.0000]
```

Example 2. The reverse-mode sensitivity is zero in the ordinary piecewise
constant regions.

```cpp
auto x = pluss::make_series({-1.2, 0.0, 3.8});
x.tracks_slope_(true);

auto y = x.trunc();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 3]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

## Gotchas And Failure Modes

- `trunc()` is not the same as `floor()` for negative values.
- It is often the best choice when you want C/C++-style toward-zero behavior.
- The transform remains discontinuous at integer boundaries.

## Summary

Use `trunc()` when the fractional part should simply be discarded toward zero.

## See Also

- [`Series::floor`](floor.md): toward negative infinity.
- [`Series::ceil`](ceil.md): toward positive infinity.
- [`Series::round`](round.md): to the nearest integer.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="triu_.html">
    <span class="label">Previous</span>
    <span class="title">triu_</span>
  </a>
  <a class="next-link" href="unbind.html">
    <span class="label">Next</span>
    <span class="title">unbind</span>
  </a>
</div>
