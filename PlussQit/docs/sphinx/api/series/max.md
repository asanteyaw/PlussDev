<!-- pluss-hand-authored -->

# Series::max

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::max() const -> Series
Series::max(std::int64_t dim) const -> Series
Series::max(std::int64_t dim, bool keepdim) const -> Series
Series::max(const Series& other) const -> Series
```

## What It Does

`max()` is one of the few `Series` methods whose meaning changes materially
with the overload you choose.

- `max()` with no arguments is a full ordered reduction
- `max(dim, keepdim)` is an ordered reduction along one axis
- `max(other)` is not a reduction at all; it is an elementwise comparison

That split matters because the shape rules, autograd behavior, and even the
mental model are different.

If you want "the largest value inside this series," stay on the reduction
overloads. If you want "compare this series against another one position by
position," the binary overload is the right surface.

## Overload Map

**`max()`**

Reduces across every element and returns one scalar `Series`.

**`max(dim)`**

Reduces along one axis and removes that axis from the result.

**`max(dim, keepdim)`**

Reduces along one axis and optionally preserves that axis as size `1`.

**`max(other)`**

Returns the elementwise winner between `self` and `other` under broadcasting.
Numerically, this is the same job as [`maximum()`](maximum.md).

## Parameters

**dim** (`std::int64_t`)

The axis to reduce for the reduction overloads.

- positive dims count from the front
- negative dims count from the back
- the dim must refer to an existing axis

**keepdim** (`bool`)

Whether the reduced axis is retained as size `1`.

- `false`: the axis disappears
- `true`: the axis stays, which is often more convenient if the result will be
  broadcast back against the original shape

**other** (`const Series&`)

The peer operand for the binary overload.

- `other` is broadcast against `self`
- the result shape is the broadcasted shape, not a reduced shape
- this overload has no scalar sibling; if the other side is a scalar, the
  clearer surface is [`maximum()`](maximum.md)

## Returns

`Series`
    A scalar, a reduced series, or a broadcasted comparison result depending on
    the overload.

## Gradient

The autograd story depends on the overload family.

### Reduction Overloads

For `max()`, `max(dim)`, and `max(dim, keepdim)`:

- only winning values receive sensitivity
- if a reduced lane has multiple equal maxima, the incoming derivative is split
  evenly across those ties
- for full reduction, the split is across every global maximum, not just within
  one intermediate axis-wise step

That last point is worth being explicit about because it is exactly what users
normally expect from a full extrema reduction.

### Binary Overload

For `max(other)`:

- the backward rule is the same as [`maximum()`](maximum.md)
- gradient flows to `self` where `self >= other`
- gradient flows to `other` where `self < other`
- at an exact tie, the full gradient goes to `self`

So the reduction overloads split ties, while the binary overload does not.
Those are different operations that happen to share one public name.

## Examples

The examples on this page build incrementally.

Example 1. With no arguments, `max()` collapses the whole series to one scalar
winner.

```cpp
auto scores = pluss::make_series({
    {1.0, 7.0, 3.0},
    {4.0, 7.0, 6.0},
});

std::cout << scores.max();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=7
```

This is a true reduction result: one 0-D `Series`, not a row or column.

Example 2. `max(dim)` reduces one axis instead of the whole object.

```cpp
std::cout << scores.max(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7, 7]
```

Here `dim = 1` means "reduce across columns," so each row contributes one
winner.

Example 3. `keepdim=true` keeps the reduced axis available for later
broadcasting.

```cpp
std::cout << scores.max(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[7],
  [7]]
```

This is the same value result as Example 2, but with a shape that still
remembers where the reduction happened.

Example 4. Negative dims are normalized from the back.

```cpp
std::cout << scores.max(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7, 7]
```

For a rank-2 series, `-1` means the last axis, so this is the same reduction
as `max(1)`.

Example 5. The binary overload is an elementwise comparison, not a reduction.

```cpp
auto floor_by_column = pluss::make_series({2.0, 6.0, 8.0});

std::cout << scores.max(floor_by_column);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 7, 8],
  [4, 7, 8]]
```

Every row of `scores` is compared against the same `[2, 6, 8]` peer under
ordinary broadcasting. No axis disappears because nothing is being reduced.

Example 6. `max(other)` is numerically the same operation as `maximum(other)`.

```cpp
std::cout << scores.maximum(floor_by_column);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 7, 8],
  [4, 7, 8]]
```

This is why it helps to think of `max(other)` as a compatibility-style spelling
that lands on the same comparison family as [`maximum()`](maximum.md).

Example 7. On a dimension reduction, ties split the derivative across the
winning lane.

```cpp
auto reduced_ties = pluss::make_series({
    {1.0, 5.0, 5.0},
    {2.0, -1.0, 2.0},
});
reduced_ties.tracks_slope_(true);

auto reduced_winners = reduced_ties.max(1);
reduced_winners.sum().reverse();

std::cout << "y=" << reduced_winners << "\n";
std::cout << "dx=" << reduced_ties.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 2]
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 0.5, 0.5],
  [0.5, 0, 0.5]]
```

The first row has two equal winners (`5` and `5`), so that row's unit upstream
derivative becomes `0.5` and `0.5`. The second row does the same thing across
its tied `2`s.

Example 8. Full reduction also splits evenly across every global winner.

```cpp
auto global_ties = pluss::make_series({
    {7.0, 0.0, 0.0},
    {7.0, 7.0, 1.0},
});
global_ties.tracks_slope_(true);

auto global_winner = global_ties.max();
global_winner.reverse();

std::cout << "y=" << global_winner << "\n";
std::cout << "dx=" << global_ties.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=7
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.33333333, 0, 0],
  [0.33333333, 0.33333333, 0]]
```

There are three global maxima, so each receives one third of the derivative.
This is the behavior you usually want from a true full reduction.

Example 9. The binary overload does not split ties; exact equality still
attributes the derivative to `self`.

```cpp
auto lhs = pluss::make_series({1.0, 5.0, 2.0});
auto rhs = pluss::make_series({4.0, 2.0, 2.0});
lhs.tracks_slope_(true);
rhs.tracks_slope_(true);

auto binary = lhs.max(rhs);
binary.sum().reverse();

std::cout << "y=" << binary << "\n";
std::cout << "dlhs=" << lhs.sensitivity() << "\n";
std::cout << "drhs=" << rhs.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 2]
dlhs=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 1]
drhs=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
```

Index `2` is the boundary case: both values are `2`, but the derivative still
belongs entirely to `lhs` because the binary rule uses the inclusive
`self >= other` boundary.

Example 10. An invalid dimension fails through the shared reduction validator.

```cpp
try {
  std::cout << scores.max(2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
reduce: dim out of range
```

That generic message is not a typo in the docs. `max(dim, ...)` uses the shared
reduction shape-normalization path, so the error text comes from the common
reduction validator rather than a `max`-specific formatter.

## Notes

- `max()` is overloaded across two different ideas: ordered reduction and
  elementwise comparison.
- `max(other)` is comparison-family behavior and is numerically equivalent to
  [`maximum(other)`](maximum.md).
- `max(other)` does not have a scalar overload. For scalar thresholds or
  scalar-vs-series comparison, prefer [`maximum()`](maximum.md).
- Ordered reduction requires an orderable dtype. Complex tensors are not valid
  reduction operands here.

## See Also

- [`Series::maximum`](maximum.md): the explicit elementwise comparison spelling.
- [`Series::minimum`](minimum.md): the elementwise lower counterpart.
- [`Series::sum`](sum.md): reduction with the same shape rules but additive
  rather than order-based.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumprod.html">
    <span class="label">Previous</span>
    <span class="title">cumprod</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
