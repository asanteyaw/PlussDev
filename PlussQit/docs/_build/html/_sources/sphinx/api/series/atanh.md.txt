<!-- pluss-hand-authored -->

# Series::atanh

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::atanh() const -> Series
```

## What It Does

`atanh()` applies the inverse hyperbolic tangent elementwise. For interior
values, it recovers `y` from `x=tanh(y)`.

## Turning A Bounded Ratio Into An Unbounded Coordinate

Hyperbolic tangent compresses the real line into `(-1,1)`. `atanh()` reverses
that compression: inputs near zero remain near zero, while inputs approaching
either boundary expand rapidly toward infinity. This is useful when a bounded
coefficient must be represented on an unrestricted real scale.

The real-domain cases are distinct:

| input | result |
| --- | --- |
| `-1 < x < 1` | finite real value |
| `x = -1` | negative infinity |
| `x = 1` | positive infinity |
| `|x| > 1` | `nan` |

## Parameters

None. Shape is preserved and each element is transformed independently.

## Returns

`Series`
    A new shape-preserving `Series`. Integral, bool, and float8 input promotes
    to a floating dtype.

## Gradient

For the open interval `|x|<1`,

```text
d atanh(x) / dx = 1 / (1 - x^2)
```

The derivative is positive, equals one at zero, and diverges at both domain
boundaries. Values close to `+-1` can therefore create very large sensitivities.

## Examples

Example 1. Known `tanh()` values map back to their unrestricted coordinates.

```cpp
auto values = pluss::make_series({
    -0.7615941559557649,
    0.0,
    0.7615941559557649,
});
std::cout << values.atanh() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 1]
```

Example 2. Interior, boundary, and invalid values remain distinguishable.

```cpp
std::cout << pluss::make_series({-1.25, -1.0, 0.0, 1.0, 1.25}).atanh() << "\n";
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, -inf, 0, inf, nan]
```

Example 3. Reverse mode shows sensitivity growth toward the boundaries.

```cpp
auto x = pluss::make_series({-0.5, 0.0, 0.5});
x.tracks_slope_(true);
x.atanh().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.3333333, 1, 1.3333333]
```

## Gotchas And Failure Modes

- Finite real output requires the open interval `(-1,1)`.
- Exact endpoints produce infinities; values beyond them produce `nan`.
- Gradients become very large near either endpoint.
- `atanh()` is not `atan()`: it reverses hyperbolic tangent, not tangent angles.

## Summary

Use `atanh()` to lift bounded values from `(-1,1)` onto the real line. Treat
the endpoints as singular boundaries in both forward and reverse computations.

## See Also

- [`Series::tanh`](tanh.md): forward bounded hyperbolic tangent.
- [`Series::asinh`](asinh.md): unrestricted inverse hyperbolic sine.
- [`Series::acosh`](acosh.md): inverse hyperbolic cosine on `[1,infinity)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="atan2.html"><span class="label">Previous</span><span class="title">atan2</span></a>
  <a class="next-link" href="baddbmm.html"><span class="label">Next</span><span class="title">baddbmm</span></a>
</div>
