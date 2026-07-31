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

This method family is intentionally broader than one signature suggests:

- the `Series` index-list overloads are the general advanced-index write path
- the scalar `std::int64_t` overload is an explicit 1D convenience
- the `ExtractSpec` initializer-list overload is a narrow convenience layer for
  common 1D and 2D write forms

The `accumulate` toggle belongs to the general advanced-index form. When it is
`false`, matching positions are replaced. When it is `true`, matching positions
are incremented instead.

One API quirk is worth saying explicitly: the method is declared `const` on the
handle, but it still mutates the underlying storage. On this surface, handle
constness does not imply value immutability.

## Parameters

**indices** (`pluss::msharp::RangeView::Of<Series>` or `std::initializer_list<Series>`)

The explicit advanced index tensors for the general overloads.

Use these when you already have integer index series and want the most direct
advanced-index write path. This is the only overload family here that supports
`accumulate=true`.

**index** (`std::int64_t`)

The scalar convenience form. This overload is intentionally 1D-only and requires
a scalar `values`.

**specs** (`std::initializer_list<ExtractSpec>`)

The constrained slice-spec convenience layer.

This overload supports exactly two handwritten convenience shapes:

- `x.index_put_({i}, scalar_value)` on 1D input
- `x.index_put_({SliceAll(), j}, column_values)` on 2D input

For anything richer, use [`extract_put_()`](extract_put_.md) or the general
Series index-list overloads.

**values** (`const Series&`)

The write source.

Accepted cardinality depends on the overload family:

- scalar convenience requires a scalar source
- 2D `ExtractSpec` column convenience requires a 1D source of length `size(0)`
- general Series index-list writes follow the backend advanced-index write rules;
  common usage is one value per indexed position, or scalar broadcast where
  supported

**accumulate** (`bool`)

- `false`: replace written positions
- `true`: add into written positions

This toggle is unsupported on the `ExtractSpec` convenience overload.

## Returns

`Series&`
    The same mutated series handle.

## Examples

Example 1. The initializer-list `Series` overload is the shortest way to do a
basic advanced-index write.

```cpp
auto vec = pluss::make_series(std::vector<double>{0, 1, 2, 3, 4}, {5});
auto idx = pluss::make_series(std::vector<std::int64_t>{1, 3}, {2});
auto picked = pluss::make_series(std::vector<double>{90, 70}, {2});

vec.index_put_({idx}, picked, false);

std::cout << vec;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 90, 2, 70, 4]
```

Example 2. The `RangeView` overload is the same engine, but useful when the
index list already lives in a container. Here `accumulate=true` adds into the
existing positions instead of replacing them.

```cpp
auto dup_idx = pluss::make_series(std::vector<std::int64_t>{1, 1, 3}, {3});
auto dup_values = pluss::make_series(std::vector<double>{10, 20, 30}, {3});
std::vector<pluss::Series> indices{dup_idx};

vec.index_put_(
    pluss::msharp::RangeView::Of<pluss::Series>(indices),
    dup_values,
    true);

std::cout << vec;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 120, 2, 100, 4]
```

Because index `1` appears twice, both `10` and `20` are accumulated into the
existing `90`.

Example 3. The scalar convenience overload is intentionally 1D-only and expects
a scalar source.

```cpp
vec.index_put_(0, pluss::codem::value(-5.0));
std::cout << vec;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-5, 120, 2, 100, 4]
```

Example 4. The 1D `ExtractSpec` convenience form accepts a single scalar index
specifier.

```cpp
vec.index_put_({std::int64_t{2}}, pluss::codem::value(555.0));
std::cout << vec;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-5, 120, 555, 100, 4]
```

Example 5. The 2D `ExtractSpec` convenience form is for whole-column
replacement: `{SliceAll(), j}` plus a length-`size(0)` source vector.

```cpp
auto mat = pluss::make_series(
    std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9},
    {3, 3});
auto col_values = pluss::make_series(std::vector<double>{30, 60, 90}, {3});

mat.index_put_({pluss::Slicing::SliceAll(), std::int64_t{2}}, col_values);

std::cout << mat;
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 2, 30],
  [4, 5, 60],
  [7, 8, 90]]
```

Example 6. `accumulate=true` is rejected on the slice-spec convenience overload,
because that overload is intentionally narrow.

```cpp
try {
  mat.index_put_({pluss::Slicing::SliceAll(), std::int64_t{1}}, col_values, true);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::index_put_(specs, value, accumulate): accumulate=true is unsupported for slice-spec indexing; use Series index list overload instead
```

## Notes

- Reach for the `Series` index-list overloads when you want the real advanced
  indexing surface, especially if `accumulate` matters.
- Reach for the `ExtractSpec` convenience overload only for the narrow 1D/2D
  shorthand it was designed for.
- For richer selector-based writes, [`extract_put_()`](extract_put_.md) is
  usually easier to read than constructing index tensors by hand.

## See Also

- [`Series::index`](index.md): the read-side advanced-index entrypoint.
- [`Series::extract_put_`](extract_put_.md): selector-based in-place writes.
- [`Series::extract_put`](extract_put.md): selector-based copy-returning writes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="clamp_max.html">
    <span class="label">Next</span>
    <span class="title">clamp_max</span>
  </a>
</div>
