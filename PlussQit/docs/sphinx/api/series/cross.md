<!-- pluss-hand-authored -->

# Series::cross

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::cross(const Series& other) const -> Series
Series::cross(const Series& other, std::int64_t dim) const -> Series
```

## What It Does

`cross()` is the **3-D vector cross product**, taken along an axis of extent
`3`:

```
cross(a, b) = [ a1*b2 - a2*b1,
                a2*b0 - a0*b2,
                a0*b1 - a1*b0 ]
```

Both operands must have the **same shape**, and the chosen axis must have length
exactly `3`. Every other axis is treated as a batch, so a `[n, 3]` input gives
`n` independent cross products. Without an explicit `dim`, the **last** axis is
used.

## Choosing The Vector Axis

The selected dimension is the coordinate dimension; every index assembled
from the remaining dimensions identifies one independent vector pair. For
shape `[2,3,4]` with `dim=1`, Pluss computes eight cross products, one for each
combination of the size-2 and size-4 coordinates. The result keeps the input
shape because three coordinates go in and three come out.

Order matters: `a.cross(b)` is the negative of `b.cross(a)`, while parallel
vectors produce zero. The direction records orientation, not similarity.

## Parameters

**other** (`const Series&`)

The second operand, the same shape as the receiver.

**dim** (`std::int64_t`, optional)

The axis to take the cross product along; it must have extent `3`. Defaults to
the last axis. Negative values count from the end.

## Returns

`Series`
    The cross product, the same shape as the inputs.

## Gradient

`cross()` is differentiable in both operands. It is bilinear, so the backward is
another cross product against the other operand (with a sign): for upstream
`g`,

```
d/da = cross(b, g)      (equivalently -cross(g, b))
d/db = cross(g, a)
```

## Examples

Example 1. The canonical basis identity `x̂ × ŷ = ẑ`.

```cpp
auto x_hat = pluss::make_series({1.0, 0.0, 0.0});
auto y_hat = pluss::make_series({0.0, 1.0, 0.0});

std::cout << x_hat.cross(y_hat);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 1]
```

Example 2. The gradient of a summed cross product.

```cpp
auto a = pluss::make_series({1.0, 0.0, 0.0});
auto b = pluss::make_series({0.0, 1.0, 0.0});
a.tracks_slope_(true);

a.cross(b).sum().reverse();

std::cout << "dL/da=" << a.sensitivity();
```

### Output

```text
dL/da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, -1]
```

For `L = sum(a × b)` with `b = ŷ`, the partials are
`[b1 - b2, b2 - b0, b0 - b1] = [1, 0, -1]`.

Example 3. `dim=0` treats columns as three-coordinate vectors.

```cpp
auto a = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {0.0, 0.0}});
auto b = pluss::make_series({{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}});
std::cout << a.cross(b, 0);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 1],
  [0, 0],
  [1, 0]]
```

The first column is `x-axis cross y-axis = z-axis`; the second is
`y-axis cross z-axis = x-axis`.

Example 4. An axis that is not length `3` is rejected.

```cpp
try {
  pluss::make_series({1.0, 2.0}).cross(pluss::make_series({3.0, 4.0}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
cross: expected dim extent == 3
```

## Gotchas And Failure Modes

- Both operands must have the **same shape** — `cross()` does not broadcast.
- The cross-product axis must have extent exactly `3`.
- Only the 3-D cross product is defined here; there is no 7-D or generalized
  form.

## Summary

`cross(other [, dim])` is the batched 3-vector cross product:

- both operands the same shape; the chosen axis (default last) has extent `3`
- other axes are batched
- differentiable in both operands (bilinear)

## See Also

- [`Series::LinearAlgAccessor::dot`](../series-linearalg/dot.md): the scalar
  companion product of two 3-vectors.
- [`Series::outer`](outer.md): the rank-1 product of two vectors.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cov.html">
    <span class="label">Previous</span>
    <span class="title">cov</span>
  </a>
  <a class="next-link" href="cummax.html">
    <span class="label">Next</span>
    <span class="title">cummax</span>
  </a>
</div>
