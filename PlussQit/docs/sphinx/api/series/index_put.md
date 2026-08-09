<!-- pluss-hand-authored -->

# Series::index_put

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::index_put(pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate) const -> Series
Series::index_put(std::initializer_list<Series> indices, const Series& values, bool accumulate) const -> Series
Series::index_put(std::int64_t index, const Series& values) const -> Series
```

## What It Does

Returns a modified copy after writing through explicit index tensors.

This is the copy-returning sibling of [`index_put_()`](index_put_.md):

- `index_put_()` mutates the existing series
- `index_put()` leaves the original alone and returns the patched copy

The overload split matters because the family actually covers two different
styles of write:

- general advanced-index writes through `Series` index tensors
- a narrow 1D scalar convenience overload

The general overloads are richer than the current signature alone suggests.
They support:

- integer position tensors
- bool masks
- scalar replacement
- one-value-per-selected-head updates
- trailing-slice pattern reuse
- additive updates through `accumulate=true`

Unlike [`index_put_()`](index_put_.md), this copy-returning family does **not**
provide an `ExtractSpec` convenience overload. If you want selector-language
copy-returning writes, use [`extract_put()`](extract_put.md).

## Parameters

**indices** (`pluss::msharp::RangeView::Of<Series>` or `std::initializer_list<Series>`)

The explicit index tensors for the general overloads.

These can be:

- integer index tensors
- bool masks
- undefined entries when routed through lower-level mixed-index machinery

The initializer-list form is the most convenient when the index tensors are
spelled inline. The `RangeView` form is the right choice when the index list
already lives in a container.

**index** (`std::int64_t`)

The scalar convenience form.

This overload is intentionally narrow: it patches one logical position in a 1D
series and expects a scalar `values` source.

**values** (`const Series&`)

The replacement data.

For the general overloads, the accepted cardinality depends on the indexed
shape:

- scalar: one value reused everywhere
- one value per selected advanced head
- one full tail pattern reused across every selected head
- one fully expanded output block

For the scalar convenience form, `values` must be scalar.

**accumulate** (`bool`)

Only present on the general overloads.

- `false`: replacement semantics
- `true`: additive semantics

## Returns

`Series`
    A new series containing the requested updates.

```{note} One-dimensional single-integer-list writes use the flat fast path
If you call `index_put({idx}, values, ...)` on a 1D series with one integer
index tensor, the public API intentionally reuses the same flat write semantics
as [`put_()`](put_.md): duplicate integer indices with `accumulate=false` keep
last-write behavior rather than rejecting the overlap.
```

## Examples

Example 1. The initializer-list overload is the shortest copy-returning form
for a basic integer index write.

```cpp
auto vec = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
auto idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2});
auto picked = pluss::make_series(std::vector<double>{90, 70}, {2});

std::cout << vec.index_put({idx}, picked, false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 90, 2, 70, 4]
```

Example 2. Because this is the copy-returning family, the original source
series is unchanged.

```cpp
std::cout << vec;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 2, 3, 4]
```

Example 3. The `RangeView` overload is useful when the index list already lives
in a container, and `accumulate=true` turns the update into an additive write.

```cpp
auto overlap_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 3}, {3});
auto overlap_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
std::vector<pluss::Series> overlap_indices{overlap_idx};

std::cout << vec.index_put(
    pluss::msharp::RangeView::Of<pluss::Series>(overlap_indices),
    overlap_values,
    true);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 31, 2, 33, 4]
```

Example 4. A bool mask write is also part of the general overload family.

```cpp
auto mask = pluss::make_series(std::vector<bool>{false, true, false, true, false}, {5});
std::cout << vec.index_put({mask}, pluss::make_series(-1.0), false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, -1, 2, -1, 4]
```

The bool mask selects positions `1` and `3`, and the scalar replacement is
reused for both.

Example 5. A non-indexed trailing slice can be supplied once and reused across
every selected head.

```cpp
auto mat = pluss::make_series(
    std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9},
    {3, 3});
auto row_idx = pluss::make_series(std::vector<std::int64_t>{0, 2}, {2});
auto tail_pattern = pluss::make_series(std::vector<double>{100, 200, 300}, {3});

std::cout << mat.index_put({row_idx}, tail_pattern, false);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[100, 200, 300],
  [4, 5, 6],
  [100, 200, 300]]
```

This is a good example of the "tail pattern" cardinality mode: the same
length-3 row is reused for each selected row index.

Example 6. The scalar convenience overload targets one 1D position, and
negative indices count from the end.

```cpp
std::cout << vec.index_put(-1, pluss::make_series(555.0));
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 2, 3, 555]
```

Example 7. On the 1D single-integer-list path, duplicate indices with
`accumulate=false` keep last-write semantics.

```cpp
std::cout << vec.index_put({overlap_idx}, overlap_values, false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 20, 2, 30, 4]
```

That is the same replace-vs-last-write behavior as [`put_()`](put_.md), because
this specific public shape reuses the 1D flat write route.

## Notes

- `index_put()` is the copy-returning explicit-index write family.
- Bool masks are part of the general overload semantics.
- The `values` cardinality rules are richer than "scalar or exact output match"; trailing-slice reuse is also supported.
- If you want the original object mutated, use [`index_put_()`](index_put_.md).

## See Also

- [`Series::index_put_`](index_put_.md): the in-place sibling.
- [`Series::put_`](put_.md): flat write semantics.
- [`Series::extract_put`](extract_put.md): selector-language copy-returning writes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="index_put_.html">
    <span class="label">Next</span>
    <span class="title">index_put_</span>
  </a>
</div>
