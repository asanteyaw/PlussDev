# TimelineIndexAccessor::timeline

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
const Timeline& TimelineIndexAccessor::timeline() const
```

## What It Does

Returns the underlying 1D `Timeline` payload without stripping away the temporal
values.

Use this when your next operation is not "index semantics" but "general temporal
semantics": formatting, field extraction, timezone conversion, rounding, or
other `Timeline`-native transforms.

In practice, `timeline()` is the escape hatch from the index surface back to the
broader time object.

That is usually the right move when the next step is about what each timestamp
is, rather than where it sits on an ordered axis.

## Returns

`const Timeline&`
    A reference to the underlying 1D temporal payload.

## Examples

Example 1. Expose the full time axis as a 1D `Timeline`.

```cpp
auto idx = pluss::timeindex::make({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});

auto axis = idx.timeline();
std::cout << pluss::timeline::print(axis) << "\n";
```

### Output

```text
Timeline(shape=[5], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:20:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00, 2024-11-01 01:20:00 ]
```

Example 2. Drop to `Timeline` semantics, do a transform there, then rebuild an
index if you need ordered lookup semantics again.

```cpp
auto local_axis = idx.timeline().floor("H");
auto local_idx = pluss::timeindex::make(local_axis);
std::cout << pluss::timeline::print(local_axis) << "\n";
```

## Notes

- `timeline()` does not copy the time values by itself; it exposes the stored
  payload.
- The returned object is still 1D because `TimelineIndexAccessor` itself is 1D.
- Staying on `Timeline` makes sense for formatting, calendar extraction,
  timezone transforms, and temporal arithmetic.
- If you need ordered lookup again after a `Timeline` transformation, rebuild a
  `TimelineIndexAccessor` from the transformed timeline.

## See Also

- [`TimelineIndexAccessor::searchsorted`](searchsorted.md): ordered insertion lookup on the
  index axis.
- [`TimelineIndexAccessor::slice_time`](slice_time.md): label-based time slicing without
  dropping down to `Timeline`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="to_note_iso.html">
    <span class="label">Next</span>
    <span class="title">to_note_iso</span>
  </a>
</div>
