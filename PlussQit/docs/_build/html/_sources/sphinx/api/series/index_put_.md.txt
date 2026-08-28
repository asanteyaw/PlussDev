<!-- pluss-hand-authored -->

# Series::index_put_

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::index_put_(pluss::RangeView<Series> indices, const Series& values, bool accumulate) const -> Series&
Series::index_put_(std::initializer_list<Series> indices, const Series& values, bool accumulate) const -> Series&
Series::index_put_(std::int64_t index, const Series& values, bool accumulate = false) const -> Series&
Series::index_put_(std::initializer_list<ExtractSpec> specs, const Series& values, bool accumulate = false) const -> Series&
```

## What It Does

Mutates the current `Series` through explicit index-based writes and returns the
same handle.

This family is the in-place sibling of [`index_put()`](index_put.md), but it
also has one extra wrinkle: alongside the real advanced-index overloads, it
offers a very small `ExtractSpec` shorthand for a couple of convenient public
cases.

That means there are three concepts to keep straight:

- the general advanced-index write route through index `Series`
- the scalar 1D convenience route
- the narrow `ExtractSpec` shorthand route

One API quirk is worth calling out directly: these methods are declared `const`
on the handle even though they mutate the underlying storage. On this surface,
handle constness does not guarantee immutability.

## Overload Map

`index_put_({idx0, idx1, ...}, values, accumulate)`

The main advanced-index in-place write surface.

`index_put_(index_container, values, accumulate)`

The same advanced-index behavior, convenient when the indices already live in a
container.

`index_put_(i, scalar_value)`

The 1D scalar convenience overload.

`index_put_({specs...}, values, accumulate = false)`

The narrow `ExtractSpec` shorthand overload. This is not a replacement for the
full selector-language family in [`extract_put_()`](extract_put_.md); it only
exists for a couple of small handwritten patterns.

## Parameters

**indices** (`pluss::RangeView<Series>` or `std::initializer_list<Series>`)

The explicit index tensors for the general overloads.

These overloads support the real advanced-index surface:

- integer index tensors
- bool masks
- paired or grouped advanced indices
- scalar broadcast
- pattern reuse across a non-indexed trailing block
- additive writes through `accumulate=true`

**index** (`std::int64_t`)

The scalar convenience overload.

This is a single 1D logical position. Negative indices count from the end.

**specs** (`std::initializer_list<ExtractSpec>`)

The constrained shorthand overload.

This public shorthand is intentionally narrow. The verified examples show two
important use cases:

- `x.index_put_({0LL}, scalar_value)` on 1D input
- `x.index_put_({SliceAll(), j}, column_values)` on 2D input

If your selector idea is richer than that, stop forcing it through this
overload and move to [`extract_put_()`](extract_put_.md).

**values** (`const Series&`)

The source values to write.

Accepted cardinality depends on the chosen overload:

- scalar convenience expects a scalar source
- the 1D `ExtractSpec` shorthand expects a scalar source
- the 2D column shorthand expects one value per row
- the general advanced-index route supports scalar broadcast, one-per-head,
  trailing-block reuse, and expanded advanced-shape writes

**accumulate** (`bool`)

Controls overwrite versus additive writes:

- `false`: replace written positions
- `true`: add into written positions

This toggle is supported on the general advanced-index routes. The shorthand
`ExtractSpec` route deliberately refuses `accumulate=true`.

## Returns

`Series&`
    The same `Series`, after mutation.

## Examples

Example 1. The single-index-tensor overload is the shortest in-place route for
a basic advanced-index write.

```cpp
auto idx = pluss::make_series({1LL, 3LL});
auto picked = pluss::make_series({90.0, 70.0});
auto inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

inplace.index_put_({idx}, picked, false);
std::cout << inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 90, 2, 70, 4]
```

Example 2. Bool mask writes belong to the same general overload family.

```cpp
auto mask = pluss::make_series({false, true, false, true, false});
auto mask_inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

mask_inplace.index_put_({mask}, pluss::make_series(-1.0), false);
std::cout << mask_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, -1, 2, -1, 4]
```

Example 3. Passing a container of indices is convenient when the index list
already exists, and `accumulate=true` turns the write additive.

```cpp
auto overlap_idx = pluss::make_series({1LL, 1LL, 3LL});
auto overlap_values = pluss::make_series({10.0, 20.0, 30.0});
std::vector<pluss::Series> overlap_indices{overlap_idx};
auto accum_inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

accum_inplace.index_put_(overlap_indices, overlap_values, true);
std::cout << accum_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 31, 2, 33, 4]
```

Example 4. The scalar convenience overload is 1D-focused, and negative indices
count from the end.

```cpp
accum_inplace.index_put_(-1, pluss::make_series(999.0));
std::cout << accum_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 31, 2, 33, 999]
```

Example 5. The 1D `ExtractSpec` shorthand can patch a single scalar position
without manufacturing an index tensor.

```cpp
accum_inplace.index_put_({0LL}, pluss::make_series(123.0));
std::cout << accum_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[123, 31, 2, 33, 999]
```

Example 6. The 2D `ExtractSpec` shorthand is useful for full-column
replacement.

```cpp
auto column_base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
});
auto column_values = pluss::make_series({30.0, 60.0, 90.0});

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

Example 7. The shorthand overload deliberately rejects `accumulate=true`.

```cpp
try {
  column_base.index_put_(
      {pluss::Slicing::SliceAll(), std::int64_t{1}},
      column_values,
      true);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::index_put_(specs, value, accumulate): accumulate=true is unsupported for slice-spec indexing; use Series index list overload instead
```

Example 8. On the 1D single-integer-list route, duplicate positions with
`accumulate=false` keep last-write behavior.

```cpp
auto overlap_inplace = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
overlap_inplace.index_put_({overlap_idx}, overlap_values, false);
std::cout << overlap_inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 20, 2, 30, 4]
```

Example 9. Multiple index tensors can patch paired positions directly.

```cpp
auto inplace_plane = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});
auto adv_row_idx = pluss::make_series({0LL, 2LL});
auto adv_col_idx = pluss::make_series({1LL, 3LL});
auto adv_values = pluss::make_series({500.0, 700.0});

inplace_plane.index_put_({adv_row_idx, adv_col_idx}, adv_values, false);
std::cout << inplace_plane;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 500, 12, 13],
  [20, 21, 22, 23],
  [30, 31, 32, 700]]
```

Example 10. Out-of-range positions fail, and the narrow route again reports the
shared scatter error text.

```cpp
try {
  auto bad_idx = pluss::make_series({5LL});
  inplace.index_put_({bad_idx}, pluss::make_series(10.0), false);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
put: index out of range
```

Example 11. The general advanced-index route also accepts the verified `int32`
index-tensor case here.

```cpp
auto int32_idx_inplace = pluss::make_series(std::vector<std::int32_t>{1, 3}, {2});
inplace.index_put_({int32_idx_inplace}, picked, false);
std::cout << inplace;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 90, 2, 70, 4]
```

## Gotchas And Failure Modes

- The method is in-place even though the handle is `const`.
- The `ExtractSpec` shorthand is intentionally tiny. Do not treat it as a
  substitute for [`extract_put_()`](extract_put_.md).
- `accumulate=true` is valid on the general advanced-index routes, but not on
  the shorthand `ExtractSpec` route.
- The 1D single-integer-list path inherits last-write behavior for duplicate
  positions when `accumulate=false`.
- As with `index_put()`, some narrow-route failures surface `put`-style error
  text because the implementation routes through shared scatter machinery.

## Summary

Use `index_put_()` when you want real advanced-index writes to land directly
into the current `Series`. Reach for the general index-list overloads first;
they are the real API. Treat the scalar and `ExtractSpec` conveniences as small
shortcuts, not as the main mental model.

## See Also

- [`Series::index_put`](index_put.md): copy-returning sibling.
- [`Series::put_`](put_.md): flat logical writes.
- [`Series::extract_put_`](extract_put_.md): broader selector-language writes.

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
