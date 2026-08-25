<!-- pluss-hand-authored -->

# Series::minimum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::minimum(const Series& rhs) const -> Series
Series::minimum(double rhs) const -> Series
```

## What It Does

`minimum()` compares two values position by position and keeps the smaller one
at each broadcasted location.

This is the value-wise mirror of [`maximum`](maximum.md), but the same teaching
idea applies: this is an elementwise comparison, not a reduction.

Use it when you want to:

- merge two value sources by keeping the lower side
- apply a shared ceiling
- enforce per-lane upper limits through broadcasting
- reason in terms of comparisons rather than in terms of a dedicated clamp call

## Parameters

**rhs** (`const Series&` or `double`)

The right-hand operand.

When you call `minimum(const Series& rhs)`:

- `rhs` is broadcast against `self`
- both sides are treated as ordinary value sources
- the output shape is the broadcasted shape
- the output dtype follows the two-operand rules for the two `Series`

When you call `minimum(double rhs)`:

- every element of `self` compares against one shared scalar
- the scalar follows `self`'s execution path
- the method often reads like "apply a ceiling," but it is still a comparison
  operator

The everyday rule is:

- use the `Series` overload when both sides are structured data
- use the scalar overload when one side is a repeated ceiling-like value

## Returns

`Series`
    A new `Series` containing the smaller value at each broadcasted position.

## Gradient

`minimum()` is differentiable in both operands.

The backward rule is:

- gradient flows to `self` where `self <= rhs`
- gradient flows to `rhs` where `self > rhs`

The tie rule is intentionally the same as `maximum()`, not the opposite. That
gives the comparison family one consistent boundary convention.

```{note} Tie-breaking convention
When `self[i] == rhs[i]`, the full gradient still goes to `self`. The engine
uses one consistent comparison-boundary convention across
`maximum()`/`minimum()`/`clamp_min()`/`clamp_max()` rather than flipping the tie
direction between `max`-like and `min`-like operations.
```

## Examples

The examples below build from the ordinary broadcast case through scalar
ceiling behavior and the tie rule that matters in autodiff code.

Example 1. A row-shaped peer broadcasts across the leading dimension.

```cpp
auto grid = pluss::make_series({1, 5, 3, 4, 2, 8},
    {2, 3});
auto peer = pluss::make_series({2, 6, 1}, {3});

std::cout << grid.minimum(peer);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 5, 1],
  [2, 2, 1]]
```

This is the same broadcast pattern as [`maximum`](maximum.md), but each location
keeps the smaller side instead of the larger one.

Example 2. A column-shaped peer broadcasts across the trailing dimension.

```cpp
auto column_peer = pluss::make_series({2, 6}, {2, 1});

std::cout << grid.minimum(column_peer);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 2],
  [4, 2, 6]]
```

This example is worth studying because it shows how much the broadcast pattern
matters:

- row `0` compares against `2`
- row `1` compares against `6`

Example 3. The scalar overload caps every element against one shared ceiling.

```cpp
std::cout << grid.minimum(4.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4.5, 3],
  [4, 2, 4.5]]
```

Only positions above `4.5` are pulled down. This is the same numerical idea as
[`clamp_max`](clamp_max.md), but expressed as an ordinary binary comparison.

Example 4. On an integer `Series`, the scalar overload stays on the integer
path.

```cpp
auto int_grid = pluss::make_series({1, 5, 3, 4, 2, 8},
    {2, 3});

std::cout << int_grid.minimum(4.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[[1, 4, 3],
  [4, 2, 4]]
```

This verified result teaches the same quirk as `maximum()`:

- the output remains `int32`
- the scalar does not force a floating destination
- if fractional ceilings matter, start from a floating `Series`

Example 5. The tie still attributes its derivative to `self`.

```cpp
auto a = pluss::make_series({1, 5, 2}, {3});
auto b = pluss::make_series({4, 2, 2}, {3});
a.tracks_slope_(true);
b.tracks_slope_(true);

auto y = a.minimum(b);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 2]
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 1]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
```

Index `2` is again the decisive boundary case:

- both operands are `2`
- the forward result is therefore `2`
- only `a` receives the derivative because the tie belongs to `self`

## Gotchas And Failure Modes

- `minimum()` is non-mutating. The inputs are left unchanged.
- Broadcast compatibility still applies. If the two operands cannot broadcast to
  one shared shape, the call fails.
- The scalar overload follows the left-hand `Series` path. On integer input, the
  verified behavior stays integer.
- `minimum()` is not a reduction. If you want an axis-wise or global smallest
  value, use [`min`](min.md).
- If the scalar is conceptually acting as an upper bound, [`clamp_max`](clamp_max.md)
  may express intent more directly.

## Teaching Summary

Read `minimum()` in one of two ways:

- `Series` overload: "pick the smaller source at each position"
- scalar overload: "do not let any value rise above this ceiling"

## See Also

- [`Series::maximum`](maximum.md): the larger-side counterpart.
- [`Series::clamp_max`](clamp_max.md): one-sided upper bound with the same
  inclusive comparison boundary.
- [`Series::clamp_min`](clamp_min.md): one-sided lower bound from the same
  comparison family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="maximum.html">
    <span class="label">Previous</span>
    <span class="title">maximum</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
