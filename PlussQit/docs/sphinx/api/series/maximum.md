<!-- pluss-hand-authored -->

# Series::maximum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::maximum(const Series& rhs) const
Series Series::maximum(double rhs) const
Series& Series::maximum_(double rhs)
```

## What It Does

`maximum()` compares two values position by position and keeps the larger one at
each broadcasted location.

This is an elementwise comparison, not a summary. It does **not** answer
"what is the largest value anywhere in this `Series`?" It answers "for each
position, which side wins?"

That distinction matters because `maximum()`:

- preserves the broadcasted shape rather than collapsing it
- follows ordinary broadcasting rules for the `Series` overload
- participates in gradient flow, including a specific tie rule
- can act like a thresholding helper when you use the scalar overload

In day-to-day code, `maximum()` is useful whenever you want to merge two value
sources, apply a floor, or keep the stronger of two signals without changing the
shape of the data.

## Parameters

**rhs** (`const Series&` or `double`)

The right-hand operand.

When you call `maximum(const Series& rhs)`:

- `rhs` is broadcast against `self`
- neither side is privileged; both are just value sources
- the output shape is the broadcasted shape
- the output dtype follows the combined operand rules for the two `Series`

When you call `maximum(double rhs)`:

- every element of `self` is compared against the same scalar
- the scalar follows `self`'s execution path rather than becoming a separate
  visible `Series`
- the result often behaves like "apply a floor," but it is still a comparison,
  not a dedicated clamp call

The practical reading is simple:

- use the `Series` overload when both sides are data
- use the scalar overload when one side is a single repeated threshold-like
  value

## Returns

`Series`
    A new `Series` containing the larger value at each broadcasted position.

## Gradient

`maximum()` is differentiable in both operands.

The backward rule is:

- gradient flows to `self` where `self >= rhs`
- gradient flows to `rhs` where `self < rhs`

The important edge case is the tie, because ties affect the backward pass even
though the forward value is the same from either side.

```{note} Tie-breaking convention
When `self[i] == rhs[i]`, the full gradient goes to `self`, not split across
both operands and not mirrored to `rhs`. This matches the inclusive-boundary
rule used by [`clamp_min`](clamp_min.md) and [`clamp_max`](clamp_max.md), so the
comparison family behaves consistently at boundaries.
```

## In-Place Form

`maximum_()` is the in-place sibling of [`maximum()`](maximum.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.0, 5.0, 3.0}, {3});
x.maximum_(4.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 4]
```

## Examples

The examples in this section are intentionally incremental: the same method
starts with ordinary row broadcasting, then moves through column broadcasting,
scalar behavior, dtype quirks, and finally the backward tie rule.

Example 1. A row-shaped peer broadcasts across the leading dimension.

```cpp
auto grid = pluss::make_series({1, 5, 3, 4, 2, 8},
    {2, 3});
auto peer = pluss::make_series({2, 6, 1}, {3});

std::cout << grid.maximum(peer);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 6, 3],
  [4, 6, 8]]
```

Each row of `grid` compares against the same `[2, 6, 1]`. This is a common way
to express a per-column lower bound without changing mental models and calling
it a clamp.

Example 2. A column-shaped peer broadcasts across the trailing dimension
instead.

```cpp
auto column_peer = pluss::make_series({2, 6}, {2, 1});

std::cout << grid.maximum(column_peer);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 5, 3],
  [6, 6, 8]]
```

Now each row has its own repeated floor:

- row `0` compares against `2`
- row `1` compares against `6`

That is a very different broadcast from Example 1 even though both use the same
method name.

Example 3. The scalar overload compares every element against one shared value.

```cpp
std::cout << grid.maximum(4.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[4.5, 5, 4.5],
  [4.5, 4.5, 8]]
```

Only positions below `4.5` are raised. Values already above the threshold pass
through unchanged.

This is the most direct "soft floor" reading of the scalar overload.

Example 4. On an integer `Series`, the scalar overload stays on the integer
path.

```cpp
auto int_grid = pluss::make_series({1, 5, 3, 4, 2, 8},
    {2, 3});

std::cout << int_grid.maximum(4.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[[4, 5, 4],
  [4, 4, 8]]
```

This is an important verified quirk:

- the result stays `int32`
- the scalar does **not** force a floating result here
- the effective comparison value lands on the integer path

If your intent is "preserve decimal detail after thresholding," start from a
floating `Series`.

Example 5. At a tie, the output value is shared but the gradient is not.

```cpp
auto a = pluss::make_series({1, 5, 2}, {3});
auto b = pluss::make_series({4, 2, 2}, {3});
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

Index `2` is the boundary case:

- both operands are `2`
- the forward result is therefore `2`
- only `a` receives the derivative because the tie belongs to `self`

## Gotchas And Failure Modes

- `maximum()` is non-mutating. The value-returning method leaves both inputs
  unchanged.
- Broadcast compatibility still matters. If `self` and `rhs` cannot broadcast to
  one shared shape, the call fails.
- The scalar overload follows the left-hand `Series` path. On integer input,
  verified behavior stays integer, as Example 4 shows.
- `maximum()` is not a reduction. If you want "largest value along an axis" or
  "largest value in the whole `Series`," use [`max`](max.md) instead.
- If the scalar is conceptually acting as a lower bound, [`clamp_min`](clamp_min.md)
  may read more clearly in public-facing code.

## Summary

Read `maximum()` in one of two ways:

- `Series` overload: "pick the larger source at each position"
- scalar overload: "do not let any value fall below this floor"

## See Also

- [`Series::minimum`](minimum.md): the elementwise smaller-side counterpart.
- [`Series::clamp_min`](clamp_min.md): one-sided lower bound with the same
  inclusive boundary behavior.
- [`Series::clamp_max`](clamp_max.md): one-sided upper bound built from the same
  comparison family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max_pair.html">
    <span class="label">Previous</span>
    <span class="title">max_pair</span>
  </a>
  <a class="next-link" href="minimum.html">
    <span class="label">Next</span>
    <span class="title">minimum</span>
  </a>
</div>
