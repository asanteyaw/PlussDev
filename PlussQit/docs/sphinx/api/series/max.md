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

`max()` answers one of two related but importantly different questions:

- "What is the largest value?"
- "At each position, which of these two values is larger?"

The reduction overloads answer the first question. The binary overload answers
the second.

That split matters because the result shape, axis behavior, and gradient rules
change with the overload family:

- `max()` reduces everything to one scalar winner
- `max(dim)` and `max(dim, keepdim)` reduce one axis at a time
- `max(other)` does not reduce at all; it compares peers element by element

If you carry only one mental model away from this page, let it be this:
reduction `max(...)` summarizes values, while binary `max(other)` selects
winners position by position.

## Overload Map

**`max()`**

Reduces across the entire `Series` and returns one scalar `Series`.

**`max(dim)`**

Reduces one axis and removes that axis from the result shape.

**`max(dim, keepdim)`**

Reduces one axis and optionally keeps that axis as size `1`.

**`max(other)`**

Returns the elementwise larger value between `self` and `other` under
broadcasting. Forward values match [`maximum()`](maximum.md).

## Parameters

**dim** (`std::int64_t`)

The axis used by the reduction overloads.

- positive dims count from the front
- negative dims count from the back
- the dim must refer to a real axis of the source `Series`

Changing `dim` changes which lanes compete with each other. On a rank-2
`Series`, `dim = 0` compares down each column, while `dim = 1` compares across
each row.

**keepdim** (`bool`)

Controls whether the reduced axis survives in the output shape.

- `false`: remove the axis entirely
- `true`: keep the axis as size `1`

This toggle is not just cosmetic. `keepdim=true` is often the better choice
when you plan to broadcast the reduced result back against the original shape.

**other** (`const Series&`)

The peer operand for the binary overload.

- `other` is broadcast against `self`
- nothing is reduced in this overload
- the result shape is the broadcasted shape

So `max(other)` should be read as a comparison surface, not as a reduction with
an unusual second parameter.

## Returns

`Series`
    The returned `Series` depends on the overload:

    - scalar `Series` for `max()`
    - reduced `Series` for `max(dim, ...)`
    - broadcasted comparison result for `max(other)`

## Gradient

`max()` is one of the clearer places to see that forward similarity does not
guarantee identical backward behavior.

### Reduction Overloads

For `max()`, `max(dim)`, and `max(dim, keepdim)`:

- only winning maxima receive sensitivity
- if multiple equal maxima tie inside one reduced lane, the incoming
  sensitivity is split evenly across those tied winners
- for the full reduction overload, that split happens across every global
  maximum in the whole `Series`

So reduction `max(...)` is tie-sharing.

### Binary Overload

For `max(other)`:

- backward routing matches [`maximum()`](maximum.md)
- gradient flows to `self` where `self >= other`
- gradient flows to `other` where `self < other`
- at an exact tie, the full gradient stays with `self`

So binary `max(other)` is first-operand-wins at the tie boundary, not
tie-sharing.

## Examples

The examples below move from the simplest reduction to shape-sensitive
reductions, then to broadcasting behavior, and finally to gradient and failure
cases.

Example 1. `max()` with no arguments reduces the whole `Series` to one scalar
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

The result has shape `[]`, which means this is a true scalar `Series`, not a
row or column wrapper.

Example 2. `max(0)` reduces down columns, so each column contributes one
winner.

```cpp
std::cout << scores.max(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 7, 6]
```

This example is worth comparing directly with the next one because changing
`dim` changes which values compete.

Example 3. `max(1)` reduces across columns inside each row.

```cpp
std::cout << scores.max(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7, 7]
```

Now each row contributes one winner, so the output shape is `[2]` instead of
`[3]`.

Example 4. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
std::cout << scores.max(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[7],
  [7]]
```

The values match Example 3, but the retained axis makes the shape easier to
align with the original `[2,3]` layout in later broadcasting steps.

Example 5. Negative dims count from the back.

```cpp
std::cout << scores.max(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7, 7]
```

For a rank-2 `Series`, `-1` means the last axis, so this matches `max(1)`.

Example 6. `max(other)` compares element by element under broadcasting instead
of reducing anything.

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

The right-hand `Series` is broadcast across rows. Every position keeps the
larger of the two contenders.

Example 7. `max(other)` and `maximum(other)` produce the same forward values.

```cpp
std::cout << scores.maximum(floor_by_column);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 7, 8],
  [4, 7, 8]]
```

That equivalence is useful to know because it makes the overload boundary
clearer: `max(other)` belongs to the elementwise comparison family.

Example 8. On an axis reduction, tied winners split the derivative evenly.

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

Each row has two winning maxima, so the upstream unit sensitivity is split
`0.5 / 0.5` across those tied locations.

Example 9. Full reduction also splits evenly, but now across every global
winner in the entire `Series`.

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

There are three global maxima, so each one receives one third of the upstream
sensitivity.

Example 10. The binary overload does not split ties. At an equal value, the
left operand keeps the gradient.

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

The last position ties at `2`, and the derivative stays entirely with `lhs`.
That is the key backward difference between reduction `max(...)` and binary
`max(other)`.

Example 11. A reduction dim must exist. Out-of-range dims fail immediately.

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

This is one of the easiest mistakes to make when working with dynamic ranks, so
it is worth treating as part of the normal usage story.

## Gotchas And Failure Modes

- `max()` and `max(other)` are not the same kind of operation. One reduces,
  the other compares elementwise.
- `dim = 0` and `dim = 1` can produce different shapes and different winners
  even on the same data.
- `keepdim=true` changes shape only, not values.
- Reduction overloads split ties; binary `max(other)` does not.
- The binary overload follows broadcasting rules, so shape mismatches fail at
  broadcast time rather than reduction time.
- Out-of-range dims throw instead of being silently clamped.

## Teaching Summary

Use `max()` when you want the winning value itself.

- Choose `max()` for one global winner.
- Choose `max(dim)` when each row, column, or axis lane should produce its own
  winner.
- Turn on `keepdim` when that reduced result needs to stay easy to broadcast.
- Choose `max(other)` only when you want an elementwise larger-of-two
  comparison.

That distinction makes the rest of the behavior predictable: reductions change
shape and split ties, while elementwise comparisons preserve a broadcasted shape
and route tie gradients to the left operand.

## See Also

- [`Series::maximum`](maximum.md): explicit elementwise maximum surface.
- [`Series::min`](min.md): lower-bound counterpart with the same overload map.
- [`Series::argmax`](argmax.md): returns winning indices instead of winning
  values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gather.html">
    <span class="label">Previous</span>
    <span class="title">gather</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
