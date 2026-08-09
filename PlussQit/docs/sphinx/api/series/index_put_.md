<!-- pluss-hand-authored -->

# Series::index_put_

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::index_put_(pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate) const -> Series&
Series::index_put_(std::initializer_list<Series> indices, const Series& values, bool accumulate) const -> Series&
Series::index_put_(std::int64_t index, const Series& values, bool accumulate = false) const -> Series&
Series::index_put_(std::initializer_list<ExtractSpec> specs, const Series& values, bool accumulate = false) const -> Series&
```

## What It Does

Mutates the series through explicit index-based writes and returns `*this`.

This family is intentionally broader than one signature suggests:

- the `Series` index-list overloads are the real advanced-index write surface
- the scalar `std::int64_t` overload is an explicit 1D convenience
- the `ExtractSpec` initializer-list overload is a narrow shorthand for two
  common handwritten cases

The key split is not just "copy vs in-place." It is also "general index
tensors vs constrained convenience syntax."

One API quirk is worth stating directly: the method is declared `const` on the
handle, but it still mutates the underlying storage. On this surface, handle
constness does not imply value immutability.

## Parameters

**indices** (`pluss::msharp::RangeView::Of<Series>` or `std::initializer_list<Series>`)

The general explicit-index overload family.

These overloads support:

- integer index tensors
- bool masks
- scalar replacement
- one-value-per-selected-head updates
- trailing-slice pattern reuse
- additive updates through `accumulate=true`

Use these when you want the real advanced-index write path.

**index** (`std::int64_t`)

The scalar convenience overload.

This overload is intentionally 1D-only and expects a scalar `values` source.
Negative indices count from the end.

**specs** (`std::initializer_list<ExtractSpec>`)

The constrained shorthand overload.

This convenience layer supports exactly two handwritten forms:

- `x.index_put_({i}, scalar_value)` on 1D input
- `x.index_put_({SliceAll(), j}, column_values)` on 2D input

For richer selector-language writes, use [`extract_put_()`](extract_put_.md).

**values** (`const Series&`)

The write source.

Accepted cardinality depends on the overload family:

- scalar convenience requires a scalar source
- 1D `ExtractSpec` shorthand requires a scalar source
- 2D `ExtractSpec` column shorthand requires a 1D source of length `size(0)`
- general `Series` index-list writes support the richer advanced-index cardinality rules described above

**accumulate** (`bool`)

Controls replacement vs addition:

- `false`: replace written positions
- `true`: add into written positions

This toggle is unsupported on the `ExtractSpec` shorthand overload.

## Returns

`Series&`
    The same mutated series handle.

## Examples

Example 1. The initializer-list `Series` overload is the shortest way to do a
basic integer advanced-index write in place.

```cpp
auto idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2});
auto picked = pluss::make_series(std::vector<double>{90, 70}, {2});
auto inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});

inplace.index_put_({idx}, picked, false);
std::cout << inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 90, 2, 70, 4]
```

Example 2. A bool mask write is part of the same general overload family.

```cpp
auto mask = pluss::make_series(std::vector<bool>{false, true, false, true, false}, {5});
auto mask_inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});

mask_inplace.index_put_({mask}, pluss::make_series(-1.0), false);
std::cout << mask_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, -1, 2, -1, 4]
```

Example 3. The `RangeView` overload is convenient when the index list already
exists in a container, and `accumulate=true` turns the update additive.

```cpp
auto overlap_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 3}, {3});
auto overlap_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
std::vector<pluss::Series> overlap_indices{overlap_idx};
auto accum_inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});

accum_inplace.index_put_(
    pluss::msharp::RangeView::Of<pluss::Series>(overlap_indices),
    overlap_values,
    true);
std::cout << accum_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 31, 2, 33, 4]
```

Example 4. The scalar convenience overload is intentionally 1D-only, and
negative indices count from the end.

```cpp
accum_inplace.index_put_(-1, pluss::make_series(999.0));
std::cout << accum_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 31, 2, 33, 999]
```

Example 5. The 1D `ExtractSpec` shorthand accepts a single scalar index spec.

```cpp
accum_inplace.index_put_({std::int64_t{0}}, pluss::make_series(123.0));
std::cout << accum_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[123, 31, 2, 33, 999]
```

Example 6. The 2D `ExtractSpec` shorthand is for whole-column replacement:
`{SliceAll(), j}` plus a length-`size(0)` source vector.

```cpp
auto column_base = pluss::make_series(
    std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9},
    {3, 3});
auto column_values = pluss::make_series(std::vector<double>{30, 60, 90}, {3});

column_base.index_put_({pluss::Slicing::SliceAll(), std::int64_t{2}}, column_values);
std::cout << column_base;
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 30],
  [4, 5, 60],
  [7, 8, 90]]
```

Example 7. The shorthand overload deliberately refuses `accumulate=true`,
because it is meant to stay narrow and explicit.

```cpp
try {
  column_base.index_put_({pluss::Slicing::SliceAll(), std::int64_t{1}}, column_values, true);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::index_put_(specs, value, accumulate): accumulate=true is unsupported for slice-spec indexing; use Series index list overload instead
```

Example 8. On the 1D single-integer-list path, duplicate integer indices with
`accumulate=false` keep last-write behavior, just like [`put_()`](put_.md).

```cpp
auto overlap_inplace = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
overlap_inplace.index_put_({overlap_idx}, overlap_values, false);
std::cout << overlap_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 20, 2, 30, 4]
```

## Notes

- Use the `Series` index-list overloads when you want the real advanced-index surface.
- Use the scalar or `ExtractSpec` conveniences only when their narrower contract matches your intent exactly.
- Bool masks are supported on the general overloads.
- For richer selector-based writes, [`extract_put_()`](extract_put_.md) is usually clearer than manufacturing index tensors by hand.

## See Also

- [`Series::index_put`](index_put.md): copy-returning sibling.
- [`Series::put_`](put_.md): flat write semantics.
- [`Series::extract_put_`](extract_put_.md): selector-based in-place writes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_put.html">
    <span class="label">Previous</span>
    <span class="title">index_put</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
