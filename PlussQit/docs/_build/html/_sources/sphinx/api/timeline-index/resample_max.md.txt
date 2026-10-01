<!-- pluss-hand-authored -->

# TimelineIndexAccessor::resample_max

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
pluss::Series TimelineIndexAccessor::resample_max(
    const pluss::Series& values,
    std::string_view freq,
    std::int64_t step = 1) const

pluss::Series TimelineIndexAccessor::resample_max(
    const pluss::Series& values,
    const ResampleOptions& options) const
```

## What It Computes

`resample_max()` returns the greatest valid payload value observed in each time
bin. It is the interval-high operation: use it for peak load per minute,
highest temperature per day, or maximum price per trading interval.

The timestamps and payload remain separate inputs. `values[i]` is paired with
timestamp `i`, and the result contains one value for every bin spanning the
first through last timestamp. Consequently a gap in observations may create an
output lane even though no input value belongs to it.

Floating `NaN` lanes do not compete for the maximum. If every lane in a bin is
`NaN`, or if the bin has no lanes, the result is `NaN`. Integer dtype is
preserved when every bin has data; an empty bin promotes the result to
`float64`, because returning zero would falsely claim that zero was observed.

Unlike [`resample_last()`](resample_last.md), this method ignores arrival order:
it selects by value. Unlike [`rolling_max`](rolling_agg.md), it reduces each
disjoint bin to one lane rather than returning one trailing result per input
timestamp.

## Options

The concise overload uses default `ResampleOptions`. Use the options overload
when the boundary contract matters.

| Field | Meaning | Default |
|---|---|---|
| `freq` | Base frequency unit | `"D"` |
| `step` | Positive width measured in `freq` units | `1` |
| `closed` | Edge included in each bin | `ResampleClosed::Left` |
| `label` | Edge used as the bin's timestamp label | `ResampleLabel::Left` |
| `origin` | Local start-day, first-observation, or epoch anchor | `ResampleOrigin::StartDay` |
| `offset_nanos` | Signed shift applied to that anchor | `0` |

`closed` affects membership; `label` does not. `origin` and `offset_nanos`
change where the repeating grid begins. Calendar frequencies are evaluated in
the index timezone rather than pretending every civil interval has a fixed
nanosecond length.

## Examples

Example 1. Compute one maximum per hour.

```cpp
auto index = pluss::timeindex::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z", "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
});
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});

std::cout << pluss::series::print(index.resample_max(values, "H")) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3.0000, 5.0000]
```

Example 2. A two-hour width combines the observations into one peak.

```cpp
std::cout << pluss::series::print(index.resample_max(values, "H", 2)) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[5.0000]
```

Example 3. Missing floating lanes are skipped without hiding an empty bin.

```cpp
auto sparse = pluss::timeindex::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:10:00Z",
    "2024-11-01T00:20:00Z", "2024-11-01T02:00:00Z",
});
auto readings = pluss::make_series(std::vector<double>{
    std::numeric_limits<double>::quiet_NaN(), 3.0, 1.0, 7.0});

std::cout << pluss::series::print(sparse.resample_max(readings, "H")) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.0000, nan, 7.0000]
```

Example 4. Right-closed bins place an exact hour boundary in the preceding
interval.

```cpp
pluss::TimelineIndexAccessor::ResampleOptions options;
options.freq = "H";
options.closed = pluss::TimelineIndexAccessor::ResampleClosed::Right;
options.label = pluss::TimelineIndexAccessor::ResampleLabel::Right;

std::cout << pluss::series::print(index.resample_max(values, options)) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4.0000, 5.0000]
```

Example 5. Compare value selection with endpoint selection. The maximum need
not be the last observation in its bin.

```cpp
auto sample = pluss::timeindex::make({
    "2025-01-01T00:00:00Z", "2025-01-01T00:20:00Z",
    "2025-01-01T00:40:00Z",
});
auto load = pluss::make_series({2.0, 9.0, 4.0});

std::cout << pluss::series::print(sample.resample_max(load, "H")) << "\n";
std::cout << pluss::series::print(sample.resample_last(load, "H")) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[9.0000]
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[4.0000]
```

Example 6. A rank mismatch is rejected.

```cpp
try {
  (void)index.resample_max(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}), "H");
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0,
      std::string_view(e.what()).find('\n')) << "\n";
}
```

### Output

```text
values must be 1D
```

## Validation And Cost

The index must be monotonic increasing. `values` must be a same-length 1D
numeric CPU `Series`; boolean values are rejected. Invalid frequency text and
non-positive steps fail before aggregation. The operation is out of place and
uses linear work after bin construction.

## See Also

- [`TimelineIndexAccessor::resample_min`](resample_min.md)
- [`TimelineIndexAccessor::resample_last`](resample_last.md)
- [`TimelineIndexAccessor::resample_ohlc`](resample_ohlc.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample_last.html">
    <span class="label">Previous</span>
    <span class="title">resample_last</span>
  </a>
  <a class="next-link" href="resample_mean.html">
    <span class="label">Next</span>
    <span class="title">resample_mean</span>
  </a>
</div>
