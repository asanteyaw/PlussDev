<!-- pluss-hand-authored -->

# Series::floor

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::floor() const -> Series
```

## What It Does

`floor()` rounds each element downward to the greatest integer value that is
less than or equal to that element.

For positive numbers, that often reads like "drop the fraction." For negative
numbers, it is more important to think in terms of order on the number line:
the result moves toward negative infinity.

## Returns

`Series`
    A new `Series` whose elements have been rounded downward.

## Gradient

Away from integer boundaries, `floor(x)` is piecewise constant, so its local
derivative is `0`.

That means a scalar objective built from `floor()` does not propagate ordinary
value sensitivity through those lanes on the current reverse-mode surface.

## Examples

Example 1. `floor()` rounds toward negative infinity, which matters most for
negative inputs.

```cpp
pluss::series::print(std::cout, pluss::make_series({
    {-1.2, -0.1, 0.0},
    {1.2, 2.9, 3.0},
}).floor());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ -2.0000 -1.0000 0.0000 ⎤
⎣  1.0000  2.0000 3.0000 ⎦
```

Example 2. Reverse-mode sensitivity is zero through the rounded output in the
usual non-boundary case.

```cpp
auto x = pluss::make_series({-1.2, 0.0, 3.8});
x.tracks_slope_(true);

auto y = x.floor();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2, 0, 3]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

## Gotchas And Failure Modes

- `floor()` is not "truncate the decimals" for negative values.
- Values already at an integer stay unchanged.
- The transform has jump discontinuities at integers.

## Summary

Use `floor()` when values must be rounded downward in the mathematical sense,
not merely shortened toward zero.

## See Also

- [`Series::ceil`](ceil.md): the upward counterpart.
- [`Series::round`](round.md): nearest-integer rounding.
- [`Series::trunc`](trunc.md): fractional-part removal toward zero.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="float_power.html">
    <span class="label">Previous</span>
    <span class="title">float_power</span>
  </a>
  <a class="next-link" href="floor_divide.html">
    <span class="label">Next</span>
    <span class="title">floor_divide</span>
  </a>
</div>
