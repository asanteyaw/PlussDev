# Series::traverse_put

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::traverse_put(pluss::msharp::RangeView::Of<ExtractSpec> specs, const Series& values) const -> Series
```

## What It Does

Returns a modified copy of the series after writing through a
view-representable selector.

This is the value-semantic sibling of [`traverse_put_()`](traverse_put_.md). It
keeps the same strict selector gate and the same write rules, but the update is
applied to a cloned series rather than to the current object.

Use it when all of the following are true:

- you want selector-based patching
- you want the selector restricted to ordinary traversal/view patterns
- you want the original value left untouched

## Parameters

**specs** (`pluss::msharp::RangeView::Of<ExtractSpec>`)

The destination selector.

Allowed forms are exactly the view-representable subset used by
[`traverse_slice()`](traverse_slice.md) and [`traverse_put_()`](traverse_put_.md):

- scalar indices
- positive-step slices
- `SliceAll()`
- `Ellipsis`
- `NewAxis`

Anything that would require materialized advanced indexing is rejected before
the copy is patched.

**values** (`const Series&`)

The replacement data.

This method accepts either:

- a scalar source, broadcast across the selected region
- a non-scalar source whose element count exactly matches the selected region

## Returns

`Series`
    A new series containing the patched result.

## Examples

Example 1. Start from a grid that has already been modified in place through
`traverse_put_()`, then derive one more patch without changing that input.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{10, 100, 12, 13, 20, 200, 22, 23, 30, 300, -5, -5},
    {3, 4});

std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13],
  [20, 200, 22, 23],
  [30, 300, -5, -5]]
```

Example 2. The returned copy receives the new patch.

```cpp
const std::vector<ExtractSpec> first_two{
    std::int64_t{0},
    pluss::Slicing::SliceRange(0, 2, 1)};

auto copied = grid.traverse_put(
    pluss::msharp::RangeView::Of<ExtractSpec>(first_two),
    pluss::codem::value(999.0));

std::cout << copied;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[999, 999, 12, 13],
  [20, 200, 22, 23],
  [30, 300, -5, -5]]
```

Example 3. The source series remains unchanged after the copy-returning call.

```cpp
std::cout << grid;
```

### Output

```text
pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[10, 100, 12, 13],
  [20, 200, 22, 23],
  [30, 300, -5, -5]]
```

This is the entire reason to choose `traverse_put()` over
[`traverse_put_()`](traverse_put_.md): the selector rules are the same, but the
ownership outcome is different.

## Notes

- `traverse_put()` is the strict, copy-returning selector-write surface.
- Prefer it when you are building derived values and want accidental mutation
  ruled out.
- If the selector needs masking, reverse stepping, or other materializing
  behavior, use [`extract_put()`](extract_put.md) instead.
- If you want the same selector gate but in-place mutation, use
  [`traverse_put_()`](traverse_put_.md).

## See Also

- [`Series::traverse_put_`](traverse_put_.md): in-place mutation with the same
  selector restrictions.
- [`Series::extract_put`](extract_put.md): copy-returning writes without the
  view-only restriction.
- [`Series::traverse_slice`](traverse_slice.md): read-side companion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="traverse_put_.html">
    <span class="label">Previous</span>
    <span class="title">traverse_put_</span>
  </a>
  <a class="next-link" href="block.html">
    <span class="label">Next</span>
    <span class="title">block</span>
  </a>
</div>
