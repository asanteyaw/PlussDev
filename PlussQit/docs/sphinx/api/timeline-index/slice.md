# TimelineIndexAccessor::slice

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
pluss::Series TimelineIndexAccessor::slice(const Timeline& start, const Timeline& end) const
```

## What It Does

Returns the matching positional indices for an inclusive time-range slice.

This method does not return a new `TimelineIndexAccessor`. It returns a 1D integer
`Series` containing the case positions whose timestamps fall within
`[start, end]`.

That design is deliberate: `slice()` answers the question "which rows belong to
this time window?" so the result can be used directly for gather-style work in
other objects.

So the method is axis-first but payload-friendly. It resolves time membership
once, then lets you reuse the returned positions on any aligned object.

## Parameters

**start**

The inclusive lower bound. It must be a scalar `Timeline`.

Rows earlier than `start` are excluded.

**end**

The inclusive upper bound. It must be a scalar `Timeline`.

Rows later than `end` are excluded.

## Returns

`pluss::Series`
    A 1D integer `Series` of matching positions.

## Examples

Example 1. Return the positions whose timestamps fall inside an inclusive time
window.

```cpp
auto start = pluss::timeline::make("2024-11-01T00:15:00Z");
auto end = pluss::timeline::make("2024-11-01T01:00:00Z");

auto take_rows = idx.slice(start, end);
std::cout << pluss::series::print(take_rows) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 2. Use the returned integer `Series` as a gather plan for another
aligned payload.

```cpp
auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0});
auto rows = idx.slice(
    pluss::timeline::make("2024-11-01T00:15:00Z"),
    pluss::timeline::make("2024-11-01T01:00:00Z"));

auto window_values = values.take(rows);
std::cout << pluss::series::print(rows) << "\n";
std::cout << pluss::series::print(window_values) << "\n";
```

This is the normal workflow: determine the row membership on the axis, then
gather aligned values with the same positions.

## Notes

- Bounds are inclusive.
- The result is positional, not textual.
- `slice()` is the right surface when the row membership is the thing you want
  to reuse elsewhere.
- If you want string-bound convenience, use [`slice_time`](slice_time.md).

## See Also

- [`TimelineIndexAccessor::slice_time`](slice_time.md): same idea, but with ISO-like string
  bounds.
- [`TimelineIndexAccessor::searchsorted`](searchsorted.md): the lower-level insertion lookup
  primitive behind range slicing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="slice_index.html">
    <span class="label">Next</span>
    <span class="title">slice_index</span>
  </a>
</div>
