# Thread::as_timeindex

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::as_timeindex() -> const pluss::TimeIndex&
```

Expose the underlying `pluss::TimeIndex` payload directly.

`Thread::as_timeindex()` is the indexed-temporal escape hatch for resampling, calendar bucketing, and time-index-specific kernels that live below the Thread surface.

## Parameters

`None`

## Returns

`const pluss::TimeIndex&`
    Reference to the underlying indexed temporal payload.

## See Also

- <a href="as_timeline.html"><code>Thread::as_timeline</code></a>: expose a timeline payload instead of a time index.
- <a href="dataframe_api_reference.html#resample"><code>DataFrame::resample</code></a>: resample full frames by a temporal feature.

## Notes

This method is useful when the operation you want exists on `TimeIndex` but not on `Thread`.

Wrap the result back into `Thread(...)` if a downstream table API expects a thread again.

## Examples

Example 1. Use the TimeIndex payload for daily resample counts and wrap the result back into a Thread.

```cpp
using namespace pluss::table;

auto ticks = Thread(pluss::make_timeindex({
    "2024-01-01T00:10:00Z",
    "2024-01-01T00:20:00Z",
    "2024-01-01T01:10:00Z",
    "2024-01-02T00:15:00Z",
}, true));

auto out = Thread(ticks.as_timeindex().resample_count("D", 1));
```

### Output

```text
0  3
1  1
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_series.html">
    <span class="label">Previous</span>
    <span class="title">as_series</span>
  </a>
  <a class="next-link" href="as_timeline.html">
    <span class="label">Next</span>
    <span class="title">as_timeline</span>
  </a>
</div>
