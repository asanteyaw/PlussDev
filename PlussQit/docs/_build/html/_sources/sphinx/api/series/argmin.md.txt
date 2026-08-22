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

Returns the index of the smallest value rather than the value itself.

This is the index-returning companion to [`min()`](min.md):

- `min()` tells you the winning value
- `argmin()` tells you where that winning value lives

Like [`argmax()`](argmax.md), this is a reduction-only family. There is no
binary comparison overload because index selection only makes sense after
choosing a single winning lane.

## Overload Map

**`argmin()`**

Reduces across the entire series and returns one flat global index in contiguous
row-major order.

**`argmin(dim)`**

Reduces one axis and returns the local winner index inside that axis.

**`argmin(dim, keepdim)`**

Same reduction as `argmin(dim)`, with optional axis retention as size `1`.

## Parameters

**dim** (`std::int64_t`)

The axis whose local minimum index should be returned.

- positive dims count from the front
- negative dims count from the back
- the dim must exist

For the dimension overloads, returned indices are local offsets along the
reduced axis, not global flattened positions.

**keepdim** (`bool`)

Whether the reduced axis is kept in the output shape.

- `false`: remove the axis
- `true`: preserve it as size `1`

## Returns

`Series`
    An `int64` index result. The no-argument overload returns one scalar index;
    the dimension overloads return reduced `int64` series.

## Gradient

`argmin()` is not a differentiable value op.

Like [`argmax()`](argmax.md), it does not currently attach a reverse-mode
source link. Treat it as an index-selection primitive.

## Tie Policy

Ties go to the first winning position encountered.

So:

- `argmin()` chooses the first winning flat position
- `argmin(dim, ...)` chooses the first winning position along the reduced axis

This is deliberate selector behavior, not the tie-sharing semantics of
[`min()`](min.md).

## Examples

The examples on this page build incrementally.

Example 1. With no arguments, `argmin()` returns a flat global index.

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

The flattened order is `[1, -7, 3, -9, -7, 6]`, so the global minimum `-9`
appears at flat index `3`.

Example 2. `argmin(dim)` returns local lane indices instead of one flat global
offset.

```cpp
std::cout << cold.argmin(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

Row `0` wins at column `1`; row `1` wins at column `0`.

Example 3. `keepdim=true` keeps the reduced axis available for downstream shape
alignment.

```cpp
std::cout << cold.argmin(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [0]]
```

Again, the indices are unchanged; only the shape policy changes.

Example 4. Negative dims normalize from the back.

```cpp
std::cout << cold.argmin(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
```

For a rank-2 series, `-1` means the last axis, so this matches `argmin(1)`.

Example 5. Tied minima go to the first winning position.

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

The tied `-9`s in the first row choose column `1`, not column `2`, because the
selector keeps the first winner it sees.

Example 6. A scalar again has only one possible winning position.

```cpp
auto scalar = pluss::make_series(42.0);

std::cout << scalar.argmin();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

Example 7. Empty-dimension reduction fails explicitly.

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

That is the correct failure mode for an index-returning reduction: an empty
lane has no legal winning element.

## Notes

- `argmin()` returns indices, not values.
- The no-argument overload is a full flat reduction across the whole series.
- The returned dtype is `int64`.
- Ties go to the first winner.

## See Also

- [`Series::min`](min.md): the value-returning ordered-minimum counterpart.
- [`Series::argmax`](argmax.md): the upper-winner index counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argmax.html">
    <span class="label">Previous</span>
    <span class="title">argmax</span>
  </a>
  <a class="next-link" href="../../series_factory_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Series Factory API Reference</span>
  </a>
</div>
