# Series::view

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::view(pluss::msharp::RangeView::Int new_sizes) const -> Series
```

## What It Does

Reinterprets the same underlying storage with a different shape, without
copying, when the current layout is view-compatible.

In the current implementation that requirement is strict: `view()` only accepts
layouts that are already contiguous and shape-compatible. If the source is
non-contiguous, `view()` throws instead of silently materializing a copy.

That makes `view()` the "I want a true layout reinterpretation" API. If you want
"same logical result, copy if needed", use [`reshape()`](reshape.md) instead.

## Parameters

**new_sizes** (`pluss::msharp::RangeView::Int`)

The desired output shape.

`view()` requires:

- the same total element count as the source
- a shape that the source can be viewed as directly
- a contiguous source layout in the current implementation

If that contract is not met, the call throws. The most visible current failure
message is `"view: requires contiguous in v0"`.

## Returns

`Series`
    A new `Series` handle over the same storage, with `new_sizes` and freshly
    computed contiguous strides for that shape.

## Examples

Example 1. Reinterpret a contiguous `2 x 3` series as `3 x 2`.

```cpp
auto mat = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
const std::array<std::int64_t, 2> shape_3x2{{3, 2}};

std::cout << mat.view(
    std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6]]
```

Nothing was transposed here; the same flat storage is simply re-read under a new
shape. That is why the rows become `[1, 2]`, `[3, 4]`, `[5, 6]`.

Example 2. A transposed series is non-contiguous, so `view()` rejects it rather
than copying.

```cpp
try {
  auto transposed = mat.transpose(0, 1);
  std::cout << transposed.view(
      std::span<const std::int64_t>(shape_3x2.data(), shape_3x2.size()));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
view: requires contiguous in v0
```

This is the key boundary between `view()` and [`reshape()`](reshape.md): once the
layout stops being directly viewable, `view()` stays strict.

## Notes

- `view()` is the no-copy path; it is supposed to fail rather than quietly
  allocate.
- The output shape must preserve total `numel()`.
- If you already know the source may be non-contiguous, reach for
  [`reshape()`](reshape.md) directly.

## See Also

- [`Series::reshape`](reshape.md): same high-level shape change, but with a
  contiguous fallback when a true view is impossible.
- [`Series::slice`](slice.md): another view-producing layout change, but by
  range restriction instead of shape reinterpretation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sizes.html">
    <span class="label">Previous</span>
    <span class="title">sizes</span>
  </a>
  <a class="next-link" href="reshape.html">
    <span class="label">Next</span>
    <span class="title">reshape</span>
  </a>
</div>
