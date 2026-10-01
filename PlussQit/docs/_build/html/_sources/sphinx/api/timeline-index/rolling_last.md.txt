<!-- pluss-hand-authored -->

# TimelineIndexAccessor::rolling_last

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
pluss::Series TimelineIndexAccessor::rolling_last(
    const pluss::Series& values,
    const pluss::Duration& window) const
```

## The Most Recent Valid Value In Each Window

`rolling_last()` chooses the newest valid payload in every inclusive trailing
interval `[index[i] - window, index[i]]`. In fully populated data this is often
the current lane, but the distinction becomes important when the current value
is `NaN`: the method searches backward within the window for the most recent
valid observation.

The result remains aligned one-for-one with the original `TimelineIndexAccessor`. This is
not binning and does not reduce the case count. Use
[`resample_last()`](resample_last.md) when one closing value per disjoint time
bin is required.

## Contract

The payload must be a one-dimensional, same-length CPU `Series` with `int64` or
`float64` dtype. The window is one positive scalar `Duration`, not an array of
per-lane durations. The index must be monotonic increasing; equal timestamps
are allowed and retain lane order. Floating `NaN` lanes are skipped, and an
all-invalid window produces `NaN`. The output preserves payload dtype and index
length.

## Examples

```cpp
auto index = pluss::timeindex::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z", "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});
auto window = pluss::duration::minutes(45);
```

Example 1. With no invalid lanes, each current value is the latest visible
value.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
std::cout << pluss::series::print(index.rolling_last(values, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000]
```

Example 2. `NaN` is skipped, while a window with no valid value remains absent.

```cpp
auto sparse = pluss::make_series(std::vector<double>{
    std::numeric_limits<double>::quiet_NaN(), 8.0, 2.0, 9.0, 4.0});
std::cout << pluss::series::print(index.rolling_last(sparse, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 2.0000, 9.0000, 4.0000]
```

Example 3. Integer input preserves integer dtype.

```cpp
auto values_i64 =
    pluss::make_series(std::vector<std::int64_t>{8, 2, 9, 3, 7});
std::cout << pluss::series::print(index.rolling_last(
    values_i64, pluss::duration::minutes(30))) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[8, 2, 9, 3, 7]
```

Example 4. Construction may deliberately permit an unordered index, but a
trailing rolling operation rejects it because its moving-window contract would
be undefined.

```cpp
auto unordered = pluss::timeindex::make({
    "2024-11-01T00:20:00Z", "2024-11-01T00:00:00Z"}, false);
try {
  (void)unordered.rolling_last(pluss::make_series({8.0, 2.0}), window);
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0,
      std::string_view(e.what()).find('\n')) << "\n";
}
```

### Output

```text
TimelineIndexAccessor rolling operations require a monotonic increasing index
```

## Cost And Ownership

Window boundaries move forward once, so runtime is `O(n)` with `O(n)`
worst-case candidate storage. The method allocates a new contiguous result and
does not mutate the index or payload.

## See Also

- [`TimelineIndexAccessor::rolling_first`](rolling_first.md)
- [`TimelineIndexAccessor::rolling_max`](rolling_max.md)
- [`TimelineIndexAccessor::resample_last`](resample_last.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_first.html">
    <span class="label">Previous</span>
    <span class="title">rolling_first</span>
  </a>
  <a class="next-link" href="rolling_max.html">
    <span class="label">Next</span>
    <span class="title">rolling_max</span>
  </a>
</div>
