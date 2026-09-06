<!-- pluss-hand-authored -->

# Series::argmin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::argmin() const -> Series
Series::argmin(std::int64_t dim) const -> Series
Series::argmin(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

`argmin()` returns the index of the smallest value rather than the value
itself.

This is the index-returning companion to [`min()`](min.md):

- `min()` answers "what is the smallest value?"
- `argmin()` answers "where is the smallest value?"

That difference affects both dtype and interpretation:

- the result dtype is `int64`
- `argmin()` with no arguments returns one flat global index
- `argmin(dim)` returns local indices inside the reduced axis

So the central lesson is the same as with [`argmax()`](argmax.md): always keep
track of which index space the method is returning.

## Overload Map

**`argmin()`**

Reduces across the whole `Series` and returns one flat global index in
row-major traversal order.

**`argmin(dim)`**

Reduces one axis and returns the local winning index inside that axis.

**`argmin(dim, keepdim)`**

Same reduction as `argmin(dim)`, but optionally keeps the reduced axis as size
`1`.

There is no binary `argmin(other)` overload because index-returning selection
is a reduction-only concept.

## Parameters

**dim** (`std::int64_t`)

The axis whose local minimum index should be returned.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

Changing `dim` changes what the returned numbers mean.

- `argmin()` returns a flat index into the whole `Series`
- `argmin(dim)` returns local offsets along the chosen axis

Those are different answers to different questions.

**keepdim** (`bool`)

Controls whether the reduced axis remains in the output shape.

- `false`: remove the axis
- `true`: keep the axis as size `1`

This toggle helps when shape alignment matters after the reduction.

## Returns

`Series`
    An `int64` index `Series`.

    - scalar `Series` for `argmin()`
    - reduced `int64` `Series` for `argmin(dim, ...)`

## Gradient

`argmin()` is an index-selection primitive, not a differentiable value-returning
reduction.

As currently implemented, it does not attach a useful reverse-mode source link.
Treat it as control information for downstream selection, routing, or analysis.

## Tie Policy

Ties go to the first winning position encountered.

That means:

- `argmin()` chooses the first winning flat position in row-major order
- `argmin(dim, ...)` chooses the first winning position along the reduced axis

This is first-winner behavior, not tie-sharing.

## Examples

The examples below emphasize the difference between flat global indices and
axis-local indices, then cover tie handling and failure behavior.

Example 1. With no arguments, `argmin()` returns one flat global index.

```cpp
auto cold = pluss::make_series({
    {1.0, -7.0, 3.0},
    {-9.0, -7.0, 6.0},
});

std::cout << cold.argmin();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

The flattened row-major order is `[1, -7, 3, -9, -7, 6]`, so the minimum `-9`
appears at flat index `3`.

Example 2. `argmin(0)` returns the winning row index for each column.

```cpp
std::cout << cold.argmin(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
```

Column by column:

- column `0`: `-9` wins from row `1`
- column `1`: `-7` ties, so the first winner is row `0`
- column `2`: `3` wins from row `0`

Example 3. `argmin(1)` returns the winning column index for each row.

```cpp
std::cout << cold.argmin(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

These are column offsets inside each row, not row offsets inside each column.

Example 4. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
std::cout << cold.argmin(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [0]]
```

Only the shape changed. The winning local indices did not.

Example 5. Negative dims count from the back.

```cpp
std::cout << cold.argmin(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

For this rank-2 input, `-1` means the last axis, so this matches `argmin(1)`.

Example 6. Ties along a reduced axis go to the first winning position.

```cpp
auto argmin_ties = pluss::make_series({
    {-4.0, -9.0, -9.0},
    {-2.0, -2.0, 1.0},
});

std::cout << argmin_ties.argmin(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

In the first row, columns `1` and `2` tie at `-9`, and column `1` wins because
it appears first along the reduced axis.

Example 7. Global ties also choose the first winning flat position.

```cpp
auto argmin_global_ties = pluss::make_series({
    {-9.0, -9.0},
    {0.0, -9.0},
});

std::cout << argmin_global_ties.argmin();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

Several entries tie for the global minimum, but the first flat position wins.

Example 8. A scalar again has only one legal winning position.

```cpp
auto scalar = pluss::make_series(42.0);

std::cout << scalar.argmin();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

This keeps index semantics consistent even when the input rank is zero.

Example 9. The reduction dim must exist.

```cpp
try {
  std::cout << cold.argmin(2) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
argmin: dim out of range
```

This is the expected failure mode for an invalid axis request.

Example 10. Empty-dimension reduction fails explicitly because no valid winner
exists.

```cpp
auto empty_cols = pluss::series::zeros({3, 0});

try {
  std::cout << empty_cols.argmin(1) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
argmin: cannot reduce an empty dimension
```

An index reduction cannot legally return an answer when the reduction lane has
no elements at all.

## Gotchas And Failure Modes

- `argmin()` returns indices, not values.
- The no-argument overload returns one flat global index.
- `argmin(0)` and `argmin(1)` return indices in different local spaces.
- `keepdim=true` changes only the result shape.
- Ties are first-winner, not tie-sharing.
- Invalid dims and empty-dimension reductions fail explicitly.

## Summary

Use `argmin()` when you need to know where the smallest value lives.

- Choose `argmin()` for one flat global winner.
- Choose `argmin(dim)` when each lane along an axis should report its own local
  winning index.
- Keep the axis with `keepdim=true` when alignment with other reduced results
  matters.

The main habit to build is simple: always ask whether the returned numbers are
flat global positions or local axis positions.

## See Also

- [`Series::min`](min.md): returns winning values instead of indices; also hosts
  `min_pair`, which returns the winning value and its index together.
- [`Series::argmax`](argmax.md): upper-winner index counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argmax.html">
    <span class="label">Previous</span>
    <span class="title">argmax</span>
  </a>
  <a class="next-link" href="asin.html">
    <span class="label">Next</span>
    <span class="title">asin</span>
  </a>
</div>
