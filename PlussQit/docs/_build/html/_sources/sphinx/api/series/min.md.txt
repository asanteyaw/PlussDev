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

`min()` is the lower-bound counterpart to [`max()`](max.md), and it has the
same overload split:

- `min()` with no arguments is a full ordered reduction
- `min(dim, keepdim)` is an ordered reduction along one axis
- `min(other)` is an elementwise comparison against another series

That is worth stating plainly because these overloads are not variations of the
same operation. Two of them summarize values; one of them compares peers
position by position.

If the question is "what is the smallest value here?", stay on the reduction
overloads. If the question is "for each position, keep the lower of these two
inputs", use the binary overload.

## Overload Map

**`min()`**

Reduces across every element and returns one scalar `Series`.

**`min(dim)`**

Reduces one axis and removes that axis from the result.

**`min(dim, keepdim)`**

Reduces one axis and optionally keeps that axis as size `1`.

**`min(other)`**

Performs an elementwise lower-of-two comparison under broadcasting. Numerically,
this is the same job as [`minimum()`](minimum.md).

## Parameters

**dim** (`std::int64_t`)

The axis to reduce for the reduction overloads.

- positive dims count from the front
- negative dims count from the back
- the dim must refer to a real axis

**keepdim** (`bool`)

Controls whether the reduced axis survives in the output shape.

- `false`: remove the axis entirely
- `true`: keep that axis as size `1`

The `keepdim=true` result is often easier to broadcast back against the
original shape, so it is not just cosmetic.

**other** (`const Series&`)

The peer operand for the binary overload.

- `other` is broadcast against `self`
- the result shape is the broadcasted shape
- this overload does not reduce anything

## Returns

`Series`
    A scalar, a reduced series, or a broadcasted comparison result depending on
    the overload.

## Gradient

The backward rule again depends on which overload family you call.

### Reduction Overloads

For `min()`, `min(dim)`, and `min(dim, keepdim)`:

- only winning minima receive sensitivity
- if several equal minima share the same reduced lane, the incoming derivative
  is split evenly across those tied winners
- for a full reduction, the split is across every global minimum

So the reduction family is tie-sharing, not first-winner-takes-all.

### Binary Overload

For `min(other)`:

- the rule is the same as [`minimum()`](minimum.md)
- gradient flows to `self` where `self <= other`
- gradient flows to `other` where `self > other`
- at an exact tie, the derivative stays with `self`

That means the binary overload is not symmetric in backward routing at the tie
boundary even though the forward values are symmetric.

## Examples

The examples on this page build incrementally.

Example 1. With no arguments, `min()` returns one scalar winner for the whole
series.

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

This is a true full reduction result: one 0-D `Series`.

Example 2. `min(dim)` reduces one axis instead of the entire object.

```cpp
std::cout << lows.min(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

`dim = 1` means each row reduces across its columns, so each row contributes one
minimum.

Example 3. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
std::cout << lows.min(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

The values are the same as Example 2; only the retained axis differs.

Example 4. Negative dims are normalized from the back.

```cpp
std::cout << lows.min(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

For a rank-2 series, `-1` is the last axis, so this matches `min(1)`.

Example 5. `min(other)` is an elementwise comparison, not a reduction.

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

The right-hand input is broadcast across rows, and each position keeps the lower
of the two values.

Example 6. `min(other)` is numerically the same as `minimum(other)`.

```cpp
std::cout << lows.minimum(ceilings);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 1, 3],
  [2, 1, 4]]
```

So `min(other)` is best read as a compatibility-style spelling for the
elementwise comparison family, not as a cousin of the reduction overloads.

Example 7. On a dimension reduction, tied minima split the derivative evenly.

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

Each reduced row has two equal minima, so the unit upstream derivative is split
`0.5 / 0.5` across those tied winners.

Example 8. Full reduction also splits across every global minimum.

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

There are three global minima, so each receives one third of the derivative.

Example 9. An invalid dimension fails through the shared reduction validator.

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

That generic text is coming from the common reduction shape-normalization path,
not from a `min`-specific formatter.

## Notes

- `min()` spans two different ideas: ordered reduction and elementwise
  comparison.
- `min(other)` is numerically equivalent to [`minimum(other)`](minimum.md).
- Ordered reduction requires an orderable dtype; complex series are not valid
  reduction inputs here.

## See Also

- [`Series::max`](max.md): the upper-bound counterpart.
- [`Series::minimum`](minimum.md): the explicit elementwise comparison spelling.
- [`Series::argmin`](argmin.md): the index-returning ordered-minimum companion.

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
