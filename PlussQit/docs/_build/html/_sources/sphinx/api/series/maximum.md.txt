<!-- pluss-hand-authored -->

# Series::maximum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::maximum(const Series& rhs) const -> Series
Series::maximum(double rhs) const -> Series
```

## What It Does

Returns the elementwise maximum between `self` and `rhs`.

This is a comparison operator, not a reduction. It does not ask "what is the
largest value in this tensor?" It asks "at each position, which side wins?"

That distinction matters because `maximum()` participates in broadcasting and in
autograd. A scalar overload behaves like "compare every element against one
threshold-like value," while the `Series` overload behaves like "compare two
shapes under ordinary broadcasting rules."

## Parameters

**rhs** (`const Series&` or `double`)

The other operand.

For `maximum(const Series& rhs)`:

- `rhs` is broadcast against `self`
- the result dtype is the promoted dtype of the two operands
- both operands remain ordinary values; neither side is treated specially as a
  "bound"

For `maximum(double rhs)`:

- every element of `self` is compared against the same scalar
- the scalar is materialized onto `self`'s lane, so the result stays on `self`'s
  dtype/platform path rather than becoming a separate broadcasted tensor value in
  a wider promoted dtype

## Returns

`Series`
    A new series containing the winner at each broadcasted position.

## Gradient

`maximum()` is differentiable in both operands.

The backward rule is:

- gradient flows to `self` where `self >= rhs`
- gradient flows to `rhs` where `self < rhs`

The important edge case is the tie.

```{note} Tie-breaking convention
When `self[i] == rhs[i]`, the full gradient goes to `self`, not split across
both operands and not mirrored to `rhs`. This matches the inclusive-boundary
rule used by [`clamp_min`](clamp_min.md) and [`clamp_max`](clamp_max.md), so the
comparison family behaves consistently at boundaries.
```

## Examples

Example 1. The `Series` overload broadcasts a row-shaped peer across the leading
dimension.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{1, 5, 3, 4, 2, 8},
    {2, 3});
auto peer = pluss::make_series(std::vector<double>{2, 6, 1}, {3});

std::cout << grid.maximum(peer);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 6, 3],
  [4, 6, 8]]
```

Each row of `grid` is compared against the same `[2, 6, 1]` peer. This is often
the most readable form when you want a per-column floor without calling it a
clamp.

Example 2. The scalar overload compares every element against one shared value.

```cpp
std::cout << grid.maximum(4.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[4.5, 5, 4.5],
  [4.5, 4.5, 8]]
```

Only positions below `4.5` are raised. Positions already above the scalar pass
through unchanged.

Example 3. At a tie, the output is shared but the gradient is not: the tied
position attributes its derivative to `self`.

```cpp
auto a = pluss::make_series(std::vector<double>{1, 5, 2}, {3});
auto b = pluss::make_series(std::vector<double>{4, 2, 2}, {3});
a.tracks_slope_(true);
b.tracks_slope_(true);

auto y = a.maximum(b);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 2]
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 1]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
```

Index `2` is the interesting one: both operands are `2`, so both values
determine the output numerically, but only `a` receives the derivative there.

## Notes

- `maximum()` is non-mutating. The in-place sibling `maximum_()` exists, but this
  page documents the value-returning surface.
- `maximum(x, scalar)` is often the clearer spelling when the scalar is acting as
  a conceptual lower floor; [`clamp_min`](clamp_min.md) is the clearer spelling
  when you want to emphasize the one-sided-bound intent.
- This is not `amax`. If you want a reduction over an axis, use the reduction
  surface instead.

## See Also

- [`Series::minimum`](minimum.md): the elementwise counterpart. Ties still favor
  `self` there as well.
- [`Series::clamp_min`](clamp_min.md): one-sided lower bound with the same
  inclusive boundary behavior.
- [`Series::clamp_max`](clamp_max.md): one-sided upper bound built from the same
  comparison family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_select.html">
    <span class="label">Previous</span>
    <span class="title">index_select</span>
  </a>
  <a class="next-link" href="minimum.html">
    <span class="label">Next</span>
    <span class="title">minimum</span>
  </a>
</div>
