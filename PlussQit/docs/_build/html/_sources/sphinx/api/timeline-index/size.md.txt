# Timeline::TimelineIndexAccessor::size

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
std::int64_t Timeline::TimelineIndexAccessor::size() const
```

## What It Does

Returns how many timestamps are stored in the index.

This is the case-axis length. If you are resampling, joining, slicing, or
aligning another object against a `Timeline::TimelineIndexAccessor`, this is the number of original
observations on that axis.

In practice, `size()` is the first integrity check before you attach an
independent payload to the axis.

## Returns

`std::int64_t`
    The number of timestamps in the index.

## Examples

Example 1. Basic length check before resampling external values.

```cpp
auto idx = pluss::timeline::make({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
}).index();

auto n = idx.size();
std::cout << "n = " << n << "\n";
```

### Output

```text
n = 5
```

Example 2. Guard that a values vector matches the index length.

```cpp
auto values = pluss::make_series(std::vector<double>{12.0, 18.0, 15.0, 22.0});
if (values.numel() != idx.size()) {
    throw std::runtime_error("values must line up with the index");
}
```

## Notes

- `size()` is equivalent to the number of cases on the time axis.
- If a values object does not agree with `size()`, the problem is almost always
  misalignment at construction time rather than later in the pipeline.
- It is often the first sanity check before calling
  [`resample_sum`](resample_sum.md), [`resample_mean`](resample_mean.md), or
  [`rolling_sum`](rolling_sum.md).

## See Also

- [`Timeline::TimelineIndexAccessor::host`](host.md): materialize the underlying epoch-nanosecond
  ticks on the host.
- [`Timeline::TimelineIndexAccessor::timeline`](timeline.md): expose the underlying `Timeline`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="searchsorted.html">
    <span class="label">Previous</span>
    <span class="title">searchsorted</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
