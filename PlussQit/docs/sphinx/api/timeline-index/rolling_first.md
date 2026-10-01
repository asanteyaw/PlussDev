<!-- pluss-hand-authored -->

# TimelineIndexAccessor::rolling_first

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
pluss::Series TimelineIndexAccessor::rolling_first(
    const pluss::Series& values,
    const pluss::Duration& window) const
```

## The Earliest Valid Value Still Visible

`rolling_first()` returns the payload attached to the earliest timestamp that
still lies inside each trailing window. At output lane `i`, the visible
interval is `[index[i] - window, index[i]]`; among its valid payload lanes, the
method chooses the oldest one.

"First" refers to temporal position inside the current window, not the first
lane of the entire `Series`, and not the smallest value. As time advances, an
old first value remains until it crosses the inclusive left boundary. The next
valid observation then becomes first.

This is useful when a window needs a baseline or opening observation while
retaining one result per timestamp. [`resample_first()`](resample_first.md)
answers a different question: it chooses one opening value per disjoint bin and
therefore changes the output axis.

## Contract

`values` must be a same-length 1D CPU `Series` of `int64` or `float64` values.
`window` must be a positive scalar `Duration`. The index must be monotonic
increasing, although duplicate timestamps are accepted. Floating `NaN` lanes
are skipped; an all-invalid window returns `NaN`. The output preserves the
payload dtype and contains one lane per index timestamp.

## Examples

```cpp
auto index = pluss::timeindex::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z", "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});
auto window = pluss::duration::minutes(45);
```

Example 1. Watch the baseline advance as old observations expire.

```cpp
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
std::cout << pluss::series::print(index.rolling_first(values, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 2.0000, 3.0000]
```

Example 2. Invalid lanes are not candidates. The `00:20` value remains first
through `01:00`, then expires before `01:20`.

```cpp
auto sparse = pluss::make_series(std::vector<double>{
    std::numeric_limits<double>::quiet_NaN(), 8.0, 2.0, 9.0, 4.0});
std::cout << pluss::series::print(index.rolling_first(sparse, window)) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 8.0000, 8.0000, 8.0000, 2.0000]
```

Example 3. Integer endpoints remain integer.

```cpp
auto values_i64 =
    pluss::make_series(std::vector<std::int64_t>{8, 2, 9, 3, 7});
std::cout << pluss::series::print(index.rolling_first(
    values_i64, pluss::duration::minutes(30))) << "\n";
```

### Output

```text
Series(shape=[5], strides=[1], dtype=int64, platform=cpu, numel=5, contiguous=true)
data=[8, 8, 2, 9, 3]
```

Example 4. A duration array is not accepted as lane-wise window input.

```cpp
try {
  (void)index.rolling_first(
      values, pluss::duration::minutes({15, 30}));
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0,
      std::string_view(e.what()).find('\n')) << "\n";
}
```

### Output

```text
rolling_agg: window must be scalar Duration
```

## Cost And Ownership

The method advances a single window boundary and retains valid candidate
positions, giving `O(n)` time and `O(n)` worst-case auxiliary storage. It
returns a new contiguous `Series` and does not mutate either input.

## See Also

- [`TimelineIndexAccessor::rolling_last`](rolling_last.md)
- [`TimelineIndexAccessor::rolling_min`](rolling_min.md)
- [`TimelineIndexAccessor::resample_first`](resample_first.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_count.html">
    <span class="label">Previous</span>
    <span class="title">rolling_count</span>
  </a>
  <a class="next-link" href="rolling_last.html">
    <span class="label">Next</span>
    <span class="title">rolling_last</span>
  </a>
</div>
