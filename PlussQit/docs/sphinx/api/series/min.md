<!-- pluss-hand-authored -->

# Series::min

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::min() const -> Series
Series::min(std::int64_t dim) const -> Series
Series::min(std::int64_t dim, bool keepdim) const -> Series
Series::min(const Series& other) const -> Series
```

## What It Does

`min()` is the lower-bound companion to [`max()`](max.md), and it has the same
overload split:

- reduction overloads answer "what is the smallest value?"
- the binary overload answers "at each position, which input is smaller?"

That makes `min()` another method where the public name stays the same while the
mental model changes with the overload family.

- `min()` reduces the whole `Series` to one scalar winner
- `min(dim)` and `min(dim, keepdim)` reduce one axis
- `min(other)` performs an elementwise lower-of-two comparison under
  broadcasting

If you are learning to read the API fluently, it helps to separate
"ordered reduction" from "pairwise comparison" every time you see this method.

## Overload Map

**`min()`**

Reduces across the entire `Series` and returns one scalar minimum.

**`min(dim)`**

Reduces one axis and removes that axis from the output shape.

**`min(dim, keepdim)`**

Reduces one axis and optionally keeps it as size `1`.

**`min(other)`**

Returns the elementwise smaller value between `self` and `other` under
broadcasting. Forward values match [`minimum()`](minimum.md).

## Parameters

**dim** (`std::int64_t`)

The axis used by the reduction overloads.

- positive dims count from the front
- negative dims count from the back
- the dim must refer to an existing axis

Changing the dim changes which values compete to become the minimum. On a rank-2
`Series`, `dim = 0` compares within each column, while `dim = 1` compares
within each row.

**keepdim** (`bool`)

Controls shape retention for the reduced axis.

- `false`: remove the axis
- `true`: keep the axis as size `1`

This toggle matters when the reduced output will be reused in shape-sensitive
operations later on.

**other** (`const Series&`)

The peer `Series` for the binary overload.

- `other` is broadcast against `self`
- the result takes the broadcasted shape
- no axis is reduced in this overload

So `min(other)` is best understood as a comparison surface, not as a special
case of reduction.

## Returns

`Series`
    The returned `Series` is:

    - scalar for `min()`
    - reduced for `min(dim, ...)`
    - broadcasted for `min(other)`

## Gradient

Like `max()`, `min()` has different backward behavior for reduction and binary
comparison overloads.

### Reduction Overloads

For `min()`, `min(dim)`, and `min(dim, keepdim)`:

- only winning minima receive sensitivity
- equal minima inside one reduced lane split the incoming derivative evenly
- the full reduction overload splits across every global minimum in the whole
  `Series`

So reduction `min(...)` is tie-sharing.

### Binary Overload

For `min(other)`:

- backward routing matches [`minimum()`](minimum.md)
- gradient flows to `self` where `self <= other`
- gradient flows to `other` where `self > other`
- at an exact tie, the derivative stays with `self`

So the binary overload is again left-biased at the tie boundary.

## Examples

The examples below build from the simplest scalar reduction through axis-aware
reductions, then on to broadcasting, gradient behavior, and failure modes.

Example 1. `min()` with no arguments returns the single smallest value in the
whole `Series`.

```cpp
auto lows = pluss::make_series({
    {4.0, 1.0, 3.0},
    {2.0, 1.0, 6.0},
});

std::cout << lows.min();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1
```

This is a scalar `Series`, so the entire input has been reduced to one global
winner.

Example 2. `min(0)` reduces down columns.

```cpp
std::cout << lows.min(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 1, 3]
```

Each column contributes one minimum, which is why the result shape is `[3]`.

Example 3. `min(1)` reduces across columns within each row.

```cpp
std::cout << lows.min(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Now the shape is `[2]` because each row contributes one winner.

Example 4. `keepdim=true` keeps the reduced axis available as size `1`.

```cpp
std::cout << lows.min(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

The values are unchanged from Example 3. Only the shape policy changed.

Example 5. Negative dims count from the back.

```cpp
std::cout << lows.min(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

For this rank-2 input, `-1` means the last axis, so this matches `min(1)`.

Example 6. `min(other)` is an elementwise comparison under broadcasting.

```cpp
auto ceilings = pluss::make_series({3.0, 2.0, 4.0});

std::cout << lows.min(ceilings);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 1, 3],
  [2, 1, 4]]
```

The right-hand input is broadcast across rows, and each position keeps the
smaller contender.

Example 7. `min(other)` and `minimum(other)` have the same forward values.

```cpp
std::cout << lows.minimum(ceilings);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 1, 3],
  [2, 1, 4]]
```

This is a helpful equivalence because it shows that the binary overload belongs
to the elementwise comparison family, not the reduction family.

Example 8. On an axis reduction, tied minima split the derivative evenly.

```cpp
auto reduced_ties = pluss::make_series({
    {2.0, 1.0, 1.0},
    {3.0, -4.0, -4.0},
});
reduced_ties.tracks_slope_(true);

auto reduced_winners = reduced_ties.min(1);
reduced_winners.sum().reverse();

std::cout << "y=" << reduced_winners << "\n";
std::cout << "dx=" << reduced_ties.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, -4]
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 0.5, 0.5],
  [0, 0.5, 0.5]]
```

Each row has two equal minima, so each winner receives half of that row's
upstream sensitivity.

Example 9. Full reduction splits the derivative across every global minimum in
the entire `Series`.

```cpp
auto global_ties = pluss::make_series({
    {-5.0, 0.0, 0.0},
    {-5.0, -5.0, 1.0},
});
global_ties.tracks_slope_(true);

auto global_winner = global_ties.min();
global_winner.reverse();

std::cout << "y=" << global_winner << "\n";
std::cout << "dx=" << global_ties.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-5
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.33333333, 0, 0],
  [0.33333333, 0.33333333, 0]]
```

There are three global minima, so each one receives one third of the upstream
sensitivity.

Example 10. The binary overload does not split ties. At equal values, the left
operand keeps the gradient.

```cpp
auto lhs = pluss::make_series({1.0, 5.0, 2.0});
auto rhs = pluss::make_series({1.0, 2.0, 2.0});
lhs.tracks_slope_(true);
rhs.tracks_slope_(true);

auto binary = lhs.min(rhs);
binary.sum().reverse();

std::cout << "y=" << binary << "\n";
std::cout << "dlhs=" << lhs.sensitivity() << "\n";
std::cout << "drhs=" << rhs.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 2]
dlhs=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 1]
drhs=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
```

Positions `0` and `2` tie, and both gradients stay with `lhs`. That is the
main backward quirk to remember for the binary overload.

Example 11. A reduction dim must refer to a real axis.

```cpp
try {
  std::cout << lows.min(2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
reduce: dim out of range
```

This is the expected failure mode when the dim does not exist.

## Gotchas And Failure Modes

- `min()` and `min(other)` are different operation families that happen to
  share a name.
- `dim = 0` and `dim = 1` compare different lanes and usually produce
  different shapes.
- `keepdim=true` preserves the reduced axis but does not change which values
  win.
- Reduction overloads split ties evenly; binary `min(other)` sends tie
  gradients to the left operand.
- Broadcastability matters for `min(other)`.
- Out-of-range dims throw immediately.

## Summary

Use `min()` when you need the smallest value itself.

- Choose `min()` for one global minimum.
- Choose `min(dim)` when each axis lane should contribute its own local
  minimum.
- Choose `keepdim=true` when you need the reduced result to stay easy to align
  with the original shape.
- Choose `min(other)` when the task is elementwise lower-of-two selection under
  broadcasting.

Once that overload choice is clear, the rest follows naturally: reductions
change shape and share ties, while pairwise comparison preserves a broadcasted
shape and keeps tie gradients on the left operand.

## See Also

- [`Series::minimum`](minimum.md): explicit elementwise minimum surface.
- [`Series::max`](max.md): upper-bound counterpart with the same overload map.
- [`Series::argmin`](argmin.md): returns winning indices instead of winning
  values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="argmax.html">
    <span class="label">Next</span>
    <span class="title">argmax</span>
  </a>
</div>
