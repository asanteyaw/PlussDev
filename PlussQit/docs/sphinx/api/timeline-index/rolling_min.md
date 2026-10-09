<!-- pluss-hand-authored -->

# Timeline::TimelineIndexAccessor::rolling_min

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
pluss::Series Timeline::TimelineIndexAccessor::rolling_min(
    const pluss::Series& values,
    const pluss::Duration& window) const
```

## The Lowest Value Still In View

`rolling_min()` reports the smallest valid payload value visible in each
trailing time interval. For output lane `i`, membership is determined by
timestamps in `[index[i] - window, index[i]]`; the reduction is then applied to
the corresponding `values` lanes.

This answers questions such as "what was the lowest reading during the last 45
minutes?" without pretending that observations arrive at a fixed cadence. It
preserves one result per original timestamp. By contrast,
[`resample_min()`](resample_min.md) partitions time into disjoint bins and can
change the number of lanes.

## Contract

- `values` is a same-length, one-dimensional CPU `Series`.
- Supported payload dtypes are `int64` and `float64`.
- `window` is a positive scalar `Duration`; a duration vector is not a
  per-lane window specification.
- The index must be monotonic increasing. Equal timestamps are allowed.
- The left edge is inclusive.
- Floating `NaN` lanes are ignored. An all-`NaN` window returns `NaN`.
- The result preserves the payload dtype and has shape `[index.size()]`.

## Examples

```cpp
auto index = pluss::timeline::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z", "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
}).index();
auto window = pluss::duration::minutes(45);
```

Example 1. Track a trailing low over irregularly spaced observations.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
std::cout << pluss::series::print(index.rolling_min(values, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 2.0000, 3.0000]
```

Example 2. `NaN` is absent from the comparison, and an all-invalid first window
is represented explicitly.

```cpp
auto sparse = pluss::make_series(std::vector<double>{
    std::numeric_limits<double>::quiet_NaN(), 8.0, 2.0, 9.0, 4.0});
std::cout << pluss::series::print(index.rolling_min(sparse, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 2.0000, 2.0000, 2.0000]
```

Example 3. Compare minimum with the most recent value. At `01:20`, the rolling
minimum is still `2`, while the rolling last value is `4`.

```cpp
std::cout << pluss::series::print(index.rolling_min(sparse, window)) << "\n";
std::cout << pluss::series::print(index.rolling_last(sparse, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 2.0000, 2.0000, 2.0000]
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 2.0000, 9.0000, 4.0000]
```

Example 4. A non-positive window is rejected instead of becoming an accidental
single-row reduction.

```cpp
try {
  (void)index.rolling_min(values, pluss::duration::minutes(0));
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0,
      std::string_view(e.what()).find('\n')) << "\n";
}
```

### Output

```text
rolling_agg: window must be positive
```

## Cost And Ownership

A monotonic deque retains only candidates that can become a future minimum.
Every lane enters and leaves that deque at most once, giving `O(n)` time and
`O(n)` worst-case auxiliary storage. The returned contiguous `Series` owns its
storage; both inputs remain unchanged.

## See Also

- [`Timeline::TimelineIndexAccessor::rolling_max`](rolling_max.md)
- [`Timeline::TimelineIndexAccessor::rolling_first`](rolling_first.md)
- [`Timeline::TimelineIndexAccessor::resample_min`](resample_min.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_mean.html">
    <span class="label">Previous</span>
    <span class="title">rolling_mean</span>
  </a>
  <a class="next-link" href="rolling_sum.html">
    <span class="label">Next</span>
    <span class="title">rolling_sum</span>
  </a>
</div>
