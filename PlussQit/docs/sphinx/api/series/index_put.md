<!-- pluss-hand-authored -->

# Series::index_put

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series::index_put(pluss::RangeView<Series> indices, const Series& values, bool accumulate) const -> Series
Series::index_put(std::initializer_list<Series> indices, const Series& values, bool accumulate) const -> Series
Series::index_put(std::int64_t index, const Series& values) const -> Series
```

## What It Does

Returns a patched copy after writing through explicit index tensors.

This family is the copy-returning sibling of [`index_put_()`](index_put_.md):

- `index_put_()` mutates the current `Series`
- `index_put()` leaves the current `Series` unchanged and returns a new one

That ownership difference matters in pipelines and expression-heavy code, but
it is not the only thing to understand here. The overload family also covers
two different styles of write:

- general advanced-index writes through one or more index `Series`
- a narrow scalar convenience overload for a single 1D position

If you want selector-language writes like slices, `Ellipsis()`, `NewAxis()`, or
mask-and-slice combinations spelled directly at the call site, this is not the
best surface. Use [`extract_put()`](extract_put.md) instead.

## Overload Map

`index_put({idx0, idx1, ...}, values, accumulate)`

The main advanced-index write surface. Use this when you already have explicit
index tensors or want to spell them inline.

`index_put(index_container, values, accumulate)`

The same advanced-index behavior, but convenient when the index list already
lives in a container such as `std::vector<Series>`.

`index_put(i, scalar_value)`

The narrow scalar convenience overload. This is for one 1D logical position and
is not meant to replace the general advanced-index surface.

## Parameters

**indices** (`pluss::RangeView<Series>` or `std::initializer_list<Series>`)

The explicit index tensors for the general overloads.

Common forms include:

- integer index tensors
- bool masks
- multiple integer index tensors for paired advanced addressing

The initializer-list form is usually the nicest way to write small examples:
`x.index_put({rows, cols}, values, false)`. The `RangeView` form is useful when
those same index tensors were built earlier and stored in a container.

**index** (`std::int64_t`)

The scalar convenience overload.

This is a 1D logical position. Negative values count from the end.

**values** (`const Series&`)

The replacement data.

Accepted cardinality depends on the indexed region:

- scalar value: broadcast across every selected destination
- one value per selected head position
- one trailing block reused across every selected head
- a fully expanded value block when the advanced indexing shape demands it

The important point is that `index_put()` is more flexible than a
simple "scalar or exact full-shape match" rule.

**accumulate** (`bool`)

Controls overwrite versus addition on the general overloads:

- `false`: replace written positions
- `true`: add into written positions

The scalar convenience overload does not expose this toggle.

## Returns

`Series`
    A new `Series` containing the requested updates.

## Examples

Example 1. A single integer index tensor is the shortest copy-returning form
for a basic advanced-index write.

```cpp
auto vec = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
auto idx = pluss::make_series({1LL, 3LL});
auto picked = pluss::make_series({90.0, 70.0});

std::cout << vec.index_put({idx}, picked, false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 90, 2, 70, 4]
```

Example 2. Because this is the copy-returning family, the original source stays
unchanged.

```cpp
std::cout << vec;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 2, 3, 4]
```

Example 3. If the index list already lives in a container, you can pass that
container directly and still use `accumulate=true`.

```cpp
auto overlap_idx = pluss::make_series({1LL, 1LL, 3LL});
auto overlap_values = pluss::make_series({10.0, 20.0, 30.0});
std::vector<pluss::Series> overlap_indices{overlap_idx};

std::cout << vec.index_put(overlap_indices, overlap_values, true);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 31, 2, 33, 4]
```

Example 4. Bool masks are part of the general advanced-index write family.

```cpp
auto mask = pluss::make_series({false, true, false, true, false});
std::cout << vec.index_put({mask}, pluss::make_series(-1.0), false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, -1, 2, -1, 4]
```

Example 5. A trailing block can be supplied once and reused for each selected
advanced head.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
});
auto row_idx = pluss::make_series({0LL, 2LL});
auto tail_pattern = pluss::make_series({100.0, 200.0, 300.0});

std::cout << mat.index_put({row_idx}, tail_pattern, false);
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[100, 200, 300],
  [4, 5, 6],
  [100, 200, 300]]
```

This is the "reuse one whole row pattern for every chosen row" mode.

Example 6. Multiple index tensors can address paired positions directly.

```cpp
auto plane = pluss::make_series({
    {10.0, 11.0, 12.0, 13.0},
    {20.0, 21.0, 22.0, 23.0},
    {30.0, 31.0, 32.0, 33.0},
});
auto adv_row_idx = pluss::make_series({0LL, 2LL});
auto adv_col_idx = pluss::make_series({1LL, 3LL});
auto adv_values = pluss::make_series({500.0, 700.0});

std::cout << plane.index_put({adv_row_idx, adv_col_idx}, adv_values, false);
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 500, 12, 13],
  [20, 21, 22, 23],
  [30, 31, 32, 700]]
```

Example 7. The scalar convenience overload targets one 1D logical position, and
negative indices count from the end.

```cpp
std::cout << vec.index_put(-1, pluss::make_series(555.0));
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 2, 3, 555]
```

Example 8. On the 1D single-integer-list route, duplicate positions with
`accumulate=false` keep last-write behavior.

```cpp
std::cout << vec.index_put({overlap_idx}, overlap_values, false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 20, 2, 30, 4]
```

That is a specific public-path quirk worth remembering: the call collapses to
the same flat replacement behavior used by [`put_()`](put_.md).

Example 9. Out-of-range positions fail, and on this route the error text comes
from the shared flat scatter machinery.

```cpp
try {
  auto bad_idx = pluss::make_series({5LL});
  std::cout << vec.index_put({bad_idx}, pluss::make_series(10.0), false);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
put: index out of range
```

Example 10. Integer index tensors on the general overload are broader than the
flat `put_()` rule here: this `int32` index tensor is accepted.

```cpp
auto int32_idx_copy = pluss::make_series(std::vector<std::int32_t>{1, 3}, {2});
std::cout << vec.index_put({int32_idx_copy}, picked, false);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 90, 2, 70, 4]
```

## Gotchas And Failure Modes

- `index_put()` is copy-returning. If you expect the original object to change,
  you want [`index_put_()`](index_put_.md) instead.
- The 1D single-integer-list case has a practical quirk: duplicate positions
  with `accumulate=false` keep last-write behavior rather than raising an
  overlap error.
- Some failure messages on narrow routes bubble up from the shared flat scatter
  engine, so the text may mention `put`.
- Integer index dtypes are not uniform across all write surfaces. `put_()` is
  strict about `int64`, while this general advanced-index route accepts at
  least the verified `int32` case above.

## Summary

Use `index_put()` when your write is best described by explicit index tensors
but you still want value semantics. The overload family can broadcast a scalar,
zip multiple index tensors together, reuse a trailing block across selected
heads, or accumulate into repeated destinations. Use the scalar convenience
overload only when you genuinely mean "patch one 1D position."

## See Also

- [`Series::index_put_`](index_put_.md): the in-place sibling.
- [`Series::put_`](put_.md): flat logical writes.
- [`Series::extract_put`](extract_put.md): copy-returning selector-language
  writes.

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
