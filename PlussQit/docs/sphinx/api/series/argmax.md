<!-- pluss-hand-authored -->

# Series::argmax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::argmax() const
Series Series::argmax(std::int64_t dim) const
Series Series::argmax(std::int64_t dim, bool keepdim) const
```

## What It Does

`argmax()` returns the winning index, not the winning value.

That sounds simple, but it changes how you should read every overload:

- [`max()`](max.md) asks "what is the largest value?"
- `argmax()` asks "where is the largest value?"

Because the result is an index, there are two especially important ideas to
keep straight:

- the output dtype is `int64`
- the no-argument overload returns one flat global index, while the dimension
  overloads return local indices inside the reduced axis

So `argmax()` is not just "`max()` but with numbers swapped out." It is an
index-selection family with its own axis semantics and failure behavior.

## Overload Map

**`argmax()`**

Reduces across the whole `Series` and returns one flat global index in
row-major traversal order.

**`argmax(dim)`**

Reduces one axis and returns the local winning index inside that axis.

**`argmax(dim, keepdim)`**

Same axis reduction as `argmax(dim)`, but optionally keeps the reduced axis as
size `1`.

There is no binary `argmax(other)` overload because an index winner only makes
sense after selecting from one reduction lane.

## Parameters

**dim** (`std::int64_t`)

The axis whose local winning index should be returned.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

This parameter changes the meaning of the returned numbers.

- `argmax()` returns one flat offset into the whole `Series`
- `argmax(dim)` returns local offsets along that one axis

Those are different index spaces, so do not read them interchangeably.

**keepdim** (`bool`)

Controls whether the reduced axis remains in the result shape.

- `false`: remove the axis
- `true`: keep it as size `1`

This is mostly a shape-alignment tool. It does not change which indices win.

## Returns

`Series`
    An `int64` index `Series`.

    - scalar `Series` for `argmax()`
    - reduced `int64` `Series` for `argmax(dim, ...)`

## Gradient

`argmax()` is an index-producing selection primitive, not a differentiable
value reduction.

As currently implemented, it does not attach a useful reverse-mode path. In
practice, treat `argmax()` as control information for downstream logic rather
than something you differentiate through.

## Tie Policy

Ties go to the first winning position encountered.

That means:

- `argmax()` chooses the first winning flat position in row-major order
- `argmax(dim, ...)` chooses the first winning position along the reduced axis

This is first-winner selection, not tie-sharing.

## Examples

The examples below deliberately contrast global indices with axis-local indices,
because that is the most common source of confusion when first learning
`argmax()`.

Example 1. With no arguments, `argmax()` returns one flat global index.

```cpp
auto highs = pluss::make_series({
    {1.0, 7.0, 3.0},
    {9.0, 7.0, 6.0},
});

std::cout << highs.argmax();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

The flattened row-major order is `[1, 7, 3, 9, 7, 6]`, so the winning `9`
appears at flat index `3`.

Example 2. `argmax(0)` returns the winning row index for each column.

```cpp
std::cout << highs.argmax(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 1]
```

Column by column:

- column `0`: `9` wins from row `1`
- column `1`: `7` ties, so the first winner is row `0`
- column `2`: `6` wins from row `1`

Example 3. `argmax(1)` returns the winning column index for each row.

```cpp
std::cout << highs.argmax(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

This is a different index space from Example 2. The output numbers are now
column offsets, not row offsets.

Example 4. `keepdim=true` keeps the reduced axis as size `1`.

```cpp
std::cout << highs.argmax(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [0]]
```

Only the shape changed. The winning local indices are the same as in Example 3.

Example 5. Negative dims are normalized from the back.

```cpp
std::cout << highs.argmax(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

For this rank-2 input, `-1` means the last axis, so this matches `argmax(1)`.

Example 6. Ties along a reduced axis go to the first winning position.

```cpp
auto argmax_ties = pluss::make_series({
    {4.0, 9.0, 9.0},
    {2.0, 2.0, 1.0},
});

std::cout << argmax_ties.argmax(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

In the first row, columns `1` and `2` tie at `9`, and column `1` wins because
it appears first along the reduced axis.

Example 7. Global ties also follow first-winner semantics, but now in flat
row-major order.

```cpp
auto argmax_global_ties = pluss::make_series({
    {9.0, 9.0},
    {8.0, 9.0},
});

std::cout << argmax_global_ties.argmax();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

There are several equal global maxima, but the first flat position wins.

Example 8. A scalar has only one legal winning position, so the answer is `0`.

```cpp
auto scalar = pluss::make_series(42.0);

std::cout << scalar.argmax();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

This scalar convention is useful because it keeps index semantics uniform even
when rank collapses to zero.

Example 9. The reduction dim must exist.

```cpp
try {
  std::cout << highs.argmax(2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
argmax: dim out of range
```

This is the expected failure mode when the requested axis does not exist.

Example 10. Reducing an empty lane fails explicitly because there is no valid
winner to point at.

```cpp
auto empty_rows = pluss::series::zeros({0, 3});

try {
  std::cout << empty_rows.argmax(0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
argmax: cannot reduce an empty dimension
```

Index-returning reductions cannot fabricate a winner for an empty lane, so this
failure is both expected and useful.

## Gotchas And Failure Modes

- `argmax()` returns indices, not values.
- The no-argument overload returns a flat global index, not a coordinate tuple.
- `argmax(0)` and `argmax(1)` return indices in different local index spaces.
- `keepdim=true` changes shape only.
- Ties are first-winner, not tie-sharing.
- Out-of-range dims and empty-dimension reductions both fail explicitly.

## Summary

Use `argmax()` when you need to know where the largest value lives.

- Choose `argmax()` for one flat global winner.
- Choose `argmax(dim)` when each lane along an axis should produce its own
  local winning index.
- Use `keepdim=true` when the reduced index result should remain easier to align
  with other reduced outputs.

The most important habit is to always ask yourself which index space you are
working in: flat global position or local axis position.

## See Also

- [`Series::max`](max.md): returns winning values instead of indices; also hosts
  `max_pair`, which returns the winning value and its index together.
- [`Series::argmin`](argmin.md): lower-winner index counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="any.html">
    <span class="label">Previous</span>
    <span class="title">any</span>
  </a>
  <a class="next-link" href="argmin.html">
    <span class="label">Next</span>
    <span class="title">argmin</span>
  </a>
</div>
