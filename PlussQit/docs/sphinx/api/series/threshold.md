<!-- pluss-hand-authored -->

# Series::threshold

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::threshold(double threshold_val, double value) const
```

## Replacing A Lower Region

`threshold()` applies one scalar boundary independently to every lane:

$$
y_i = \begin{cases}
x_i, & x_i > \text{threshold_val},\\
\text{value}, & x_i \le \text{threshold_val}.
\end{cases}
$$

The strict `>` is the important detail. A lane exactly equal to the threshold
is replaced. This makes the operation useful for hard activation floors,
clipping a lower region to a sentinel, or constructing a piecewise constant
branch without allocating a same-shaped condition Series.

The output has the receiver's shape, dtype, and platform. Floating and signed
or unsigned integral receivers are supported; Bool and complex receivers are
rejected because an ordered numeric boundary is not meaningful for those
families. Comparison uses the supplied floating threshold without first
truncating it to an integral receiver's dtype. The replacement is cast to the
receiver dtype, so callers should choose a representable value.

## Reverse Mode And Ownership

`threshold()` allocates a new Series and does not mutate the receiver. Its
derivative with respect to each retained lane is one. Replaced lanes, including
lanes exactly on the boundary, have derivative zero because their output is
the constant `value`. The two scalar arguments are configuration values and do
not receive sensitivity.

Use [`threshold_()`](threshold_.md) only when deliberate mutation is required.

## Examples

### 1. Observe the inclusive replacement boundary

```cpp
const auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
pluss::series::print(std::cout, x.threshold(0.5, -9.0));
```

#### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-9.0000, -9.0000, -9.0000, 1.0000]
```

The `0.5` lane is replaced because retention requires a value strictly greater
than `0.5`.

### 2. Compare integral lanes against a fractional boundary

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1, 0, 1}).threshold(-0.5, 9.5));
```

#### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[9, 0, 1]
```

The comparison remains against `-0.5`, so zero is retained. The replacement
`9.5` is converted to the receiver's `int64` dtype and stored as `9`.

### 3. Preserve higher-rank shape

```cpp
const auto matrix = pluss::make_series({{-2.0, 0.0}, {2.0, 4.0}});
pluss::series::print(std::cout, matrix.threshold(1.0, 10.0));
```

#### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 10.0000 10.0000 ⎤
⎣  2.0000  4.0000 ⎦
```

Thresholding is elementwise; no axis is reduced or broadcast.

### 4. Inspect the piecewise reverse rule

```cpp
auto tracked = x.clone();
tracked.tracks_slope_(true);
tracked.threshold(0.5, -9.0).sum().reverse();
pluss::series::print(std::cout, tracked.sensitivity());
```

#### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.0000, 0.0000, 1.0000]
```

### 5. Reject an unordered receiver dtype

```cpp
try {
  (void)pluss::make_series({true, false}).threshold(0.0, 1.0);
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
threshold: bool input is not supported
```

## See Also

- [`threshold_()`](threshold_.md) performs the same rule in place.
- [`clamp_min()`](clamp_min.md) and [`clamp_max()`](clamp_max.md) impose one-sided numeric bounds.
- [`where()`](where.md) selects between two general Series branches.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tensordot.html">
    <span class="label">Previous</span>
    <span class="title">tensordot</span>
  </a>
  <a class="next-link" href="threshold_.html">
    <span class="label">Next</span>
    <span class="title">threshold_</span>
  </a>
</div>
