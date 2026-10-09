<!-- pluss-hand-authored -->

# Timeline::TimelineIndexAccessor::rolling_agg

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
pluss::Series Timeline::TimelineIndexAccessor::rolling_agg(
    const pluss::Series& values,
    const pluss::Duration& window,
    std::string_view op) const
```

## Runtime Selection Of A Trailing Reduction

`rolling_agg()` is the dynamic entry point for the Timeline::TimelineIndexAccessor rolling family.
It applies the reducer named by `op` to every inclusive trailing interval
`[index[i] - window, index[i]]` and returns one result per original timestamp.

Use this method when the reducer is data-driven, for example when a command-line
option or configuration file chooses between `"mean"`, `"min"`, and `"max"`.
When the reduction is fixed in source code, prefer the corresponding named
method. `rolling_min(values, window)` is checked by the compiler and states its
purpose more directly than `rolling_agg(values, window, "min")`.

## Supported Operations

| `op` | Reduction | Dedicated method |
|---|---|---|
| `"count"` | Number of timestamps in the window; `values` is validated but not reduced | [`rolling_count()`](rolling_count.md) |
| `"first"` | Earliest valid payload still visible | [`rolling_first()`](rolling_first.md) |
| `"last"` | Most recent valid payload still visible | [`rolling_last()`](rolling_last.md) |
| `"max"` | Largest valid payload | [`rolling_max()`](rolling_max.md) |
| `"mean"` | Arithmetic mean of valid payloads | [`rolling_mean()`](rolling_mean.md) |
| `"min"` | Smallest valid payload | [`rolling_min()`](rolling_min.md) |
| `"sum"` | Sum of payloads | [`rolling_sum()`](rolling_sum.md) |

Names are exact and case-sensitive. An unrecognized name is an error; there is
no fallback reducer.

## Shared Contract

`values` must be one-dimensional and aligned lane-for-lane with the index. The
window must be one positive scalar `Duration`, and the index must be monotonic
increasing. Named extrema, endpoint, and mean reducers accept CPU `int64` and
`float64` payloads. Floating `NaN` lanes are skipped by those reducers, and an
all-invalid window returns `NaN`.

The left boundary is inclusive: a lane whose timestamp is exactly one window
length behind the current timestamp remains visible. Resampling differs
fundamentally: rolling preserves the original axis and overlaps windows;
resampling creates non-overlapping bins and may change output length.

## Examples

```cpp
auto index = pluss::timeline::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z", "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
}).index();
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
auto window = pluss::duration::minutes(45);
```

Example 1. Select a mean at runtime.

```cpp
std::string reducer = "mean";
std::cout << pluss::series::print(
    index.rolling_agg(values, window, reducer)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.5000, 2.0000, 3.0000, 4.0000]
```

Example 2. Change only the reducer, not window membership.

```cpp
std::cout << pluss::series::print(
    index.rolling_agg(values, window, "min")) << "\n";
std::cout << pluss::series::print(
    index.rolling_agg(values, window, "max")) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 2.0000, 3.0000]
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000]
```

Example 3. `first` and `last` are temporal endpoints, not extrema.

```cpp
std::cout << pluss::series::print(
    index.rolling_agg(values, window, "first")) << "\n";
std::cout << pluss::series::print(
    index.rolling_agg(values, window, "last")) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 2.0000, 3.0000]
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000]
```

Example 4. A misspelled or unsupported reducer fails immediately.

```cpp
try {
  (void)index.rolling_agg(values, window, "median");
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0,
      std::string_view(e.what()).find('\n')) << "\n";
}
```

### Output

```text
rolling_agg: unknown op
```

## Cost And Ownership

The named reducers use moving-window kernels. Extrema use monotonic deques;
means and endpoints advance their boundaries without rescanning prior lanes.
Their runtime is `O(n)` with `O(n)` worst-case window storage. The result is a
new contiguous `Series`; neither input is mutated.

## See Also

- [`Timeline::TimelineIndexAccessor::rolling_mean`](rolling_mean.md)
- [`Timeline::TimelineIndexAccessor::rolling_min`](rolling_min.md)
- [`Timeline::TimelineIndexAccessor::resample_agg`](resample_agg.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample_sum.html">
    <span class="label">Previous</span>
    <span class="title">resample_sum</span>
  </a>
  <a class="next-link" href="rolling_count.html">
    <span class="label">Next</span>
    <span class="title">rolling_count</span>
  </a>
</div>
