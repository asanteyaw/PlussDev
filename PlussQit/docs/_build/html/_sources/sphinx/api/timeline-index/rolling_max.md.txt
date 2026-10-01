<!-- pluss-hand-authored -->

# TimelineIndexAccessor::rolling_max

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
pluss::Series TimelineIndexAccessor::rolling_max(
    const pluss::Series& values,
    const pluss::Duration& window) const
```

## The Highest Value Still In View

`rolling_max()` computes a trailing high at every timestamp. Output lane `i`
is the largest valid payload value whose timestamp lies in the inclusive
interval `[index[i] - window, index[i]]`.

The operation is shape preserving. It does not form hourly or daily buckets,
and it does not assume an observation count. This makes it suitable for peaks
on irregular event streams. Use [`resample_max()`](resample_max.md) when each
observation must instead belong to exactly one fixed bin.

## Contract

The payload must be a same-length 1D CPU `Series` with `int64` or `float64`
dtype. The window must be a positive scalar `Duration`, and the index must be
monotonic increasing. The left boundary is inclusive, repeated timestamps are
separate observations, and floating `NaN` lanes are skipped. A window with no
valid payload returns `NaN`. The output preserves payload dtype and index
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

Example 1. Compute the trailing high.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
std::cout << pluss::series::print(index.rolling_max(values, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000]
```

Example 2. A prior peak remains until its timestamp crosses the left boundary.

```cpp
auto sparse = pluss::make_series(std::vector<double>{
    std::numeric_limits<double>::quiet_NaN(), 8.0, 2.0, 9.0, 4.0});
std::cout << pluss::series::print(index.rolling_max(sparse, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 8.0000, 9.0000, 9.0000]
```

Example 3. Maximum is not "last". A window can retain an earlier peak even
though a newer, smaller observation exists.

```cpp
std::cout << pluss::series::print(index.rolling_max(sparse, window)) << "\n";
std::cout << pluss::series::print(index.rolling_last(sparse, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 8.0000, 9.0000, 9.0000]
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 2.0000, 9.0000, 4.0000]
```

Example 4. An unordered axis cannot define a forward-moving trailing window.

```cpp
auto unordered = pluss::timeindex::make({
    "2024-11-01T00:20:00Z", "2024-11-01T00:00:00Z"}, false);
try {
  (void)unordered.rolling_max(pluss::make_series({8.0, 2.0}), window);
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

The method uses a monotonic candidate deque, so it is `O(n)` rather than
rescanning every window. Worst-case auxiliary storage is `O(n)`. The output is
a newly allocated contiguous `Series`; no input is mutated.

## See Also

- [`TimelineIndexAccessor::rolling_min`](rolling_min.md)
- [`TimelineIndexAccessor::rolling_last`](rolling_last.md)
- [`TimelineIndexAccessor::resample_max`](resample_max.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_last.html">
    <span class="label">Previous</span>
    <span class="title">rolling_last</span>
  </a>
  <a class="next-link" href="rolling_mean.html">
    <span class="label">Next</span>
    <span class="title">rolling_mean</span>
  </a>
</div>
