# TimelineIndexAccessor::resample_count

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
pluss::Series TimelineIndexAccessor::resample_count(const ResampleOptions& options) const
pluss::Series TimelineIndexAccessor::resample_count(std::string_view freq, std::int64_t step = 1) const
```

## What It Does

Counts how many observations land in each resampling bin.

This is the "size per bucket" form of resampling. It does not require a second
values vector because it counts the timestamps themselves.

It is often the best first diagnostic on a new time axis because it tells you
whether the regrouping logic itself is producing the buckets you think it is.

## Parameters

**freq**

The base bin frequency such as `"H"`, `"D"`, `"min"`, or `"S"`.

**step**

How many `freq` units each bin spans.

**options**

The full resampling control block. See
[`resample_bins`](resample_bins.md) for the detailed meaning of `closed`,
`label`, `origin`, and `offset_nanos`.

If the counts already look wrong, the problem is usually the axis or the
resample options rather than the later value aggregation.

## Returns

`pluss::Series`
    A 1D numeric `Series` containing one count per resample bin.

## Examples

Example 1. Count how many observations land in each hourly bin.

```cpp
auto counts = idx.resample_count("H");
std::cout << pluss::series::print(counts) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[3, 2]
```

Example 2. Increase `step` when you want each bin to span multiple base
frequency units.

```cpp
auto counts = idx.resample_count("H", 2);
std::cout << pluss::series::print(counts) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[5]
```

## Notes

- `resample_count()` is the direct answer to "how many rows did I observe per
  interval?"
- It is axis-only, so no secondary payload is needed.
- If you also need output labels, pair it with
  [`resample_bins`](resample_bins.md).

## See Also

- [`TimelineIndexAccessor::resample_bins`](resample_bins.md)
- [`TimelineIndexAccessor::resample_sum`](resample_sum.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample_bins.html">
    <span class="label">Previous</span>
    <span class="title">resample_bins</span>
  </a>
  <a class="next-link" href="resample_first.html">
    <span class="label">Next</span>
    <span class="title">resample_first</span>
  </a>
</div>
