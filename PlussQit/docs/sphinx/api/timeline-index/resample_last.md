<!-- pluss-hand-authored -->

# Timeline::TimelineIndexAccessor::resample_last

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signatures

```cpp
pluss::Series Timeline::TimelineIndexAccessor::resample_last(
    const pluss::Series& values,
    std::string_view freq,
    std::int64_t step = 1) const

pluss::Series Timeline::TimelineIndexAccessor::resample_last(
    const pluss::Series& values,
    const ResampleOptions& options) const
```

## Last Means Final Valid Observation

`resample_last()` selects the payload attached to the latest valid observation
inside each bin. It answers “what was the final known state during this
interval?”, which is different from asking for the greatest value. A closing
reading may be lower than the interval peak and still be the correct result.

Chronology comes from the monotonic increasing `Timeline::TimelineIndexAccessor`. `values[i]` belongs
to timestamp `i`, and the output contains one lane per generated bin. Floating
`NaN` lanes are ignored, so a trailing missing reading does not erase the last
valid state. A bin with no valid lane returns `NaN`.

Integer values remain integer when every bin is occupied. If the bin grid
contains a gap, the output becomes `float64` so the gap can be represented as
`NaN` rather than a fabricated zero.

## Options

| Field | Meaning | Default |
|---|---|---|
| `freq` | Base frequency unit | `"D"` |
| `step` | Positive number of base units per bin | `1` |
| `closed` | Boundary side included in a bin | `ResampleClosed::Left` |
| `label` | Boundary side used to label a bin | `ResampleLabel::Left` |
| `origin` | Repeating-grid anchor | `ResampleOrigin::StartDay` |
| `offset_nanos` | Signed nanosecond shift from the anchor | `0` |

The `freq, step` overload is shorthand for the defaults above. The options
overload is necessary when an exact boundary must belong to the preceding bin,
when right-edge labels are desired, or when bins must align to a different
origin. Calendar boundaries use local civil time in the index timezone.

## Examples

Example 1. Select the closing reading in each hour.

```cpp
auto index = pluss::timeline::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z", "2024-11-01T01:00:00Z",
    "2024-11-01T01:20:00Z",
}).index();
auto values = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});

std::cout << pluss::series::print(index.resample_last(values, "H")) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3.0000, 5.0000]
```

Example 2. Two-hour bins return the final value across the combined interval.

```cpp
std::cout << pluss::series::print(index.resample_last(values, "H", 2)) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[5.0000]
```

Example 3. Missing values are skipped, and an unobserved hour remains absent.

```cpp
auto sparse = pluss::timeline::make({
    "2024-11-01T00:00:00Z", "2024-11-01T00:10:00Z",
    "2024-11-01T00:20:00Z", "2024-11-01T02:00:00Z",
}).index();
auto readings = pluss::make_series(std::vector<double>{
    std::numeric_limits<double>::quiet_NaN(), 3.0, 1.0, 7.0});

std::cout << pluss::series::print(sparse.resample_last(readings, "H")) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, nan, 7.0000]
```

Example 4. Contrast closing state with interval peak.

```cpp
auto sample = pluss::timeline::make({
    "2025-01-01T00:00:00Z", "2025-01-01T00:20:00Z",
    "2025-01-01T00:40:00Z",
}).index();
auto load = pluss::make_series({2.0, 9.0, 4.0});

std::cout << pluss::series::print(sample.resample_last(load, "H")) << "\n";
std::cout << pluss::series::print(sample.resample_max(load, "H")) << "\n";
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[4.0000]
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[9.0000]
```

Example 5. Right closure moves the exact `01:00` value into the first bin, so
that value becomes the first bin's close.

```cpp
pluss::Timeline::TimelineIndexAccessor::ResampleOptions options;
options.freq = "H";
options.closed = pluss::Timeline::TimelineIndexAccessor::ResampleClosed::Right;
options.label = pluss::Timeline::TimelineIndexAccessor::ResampleLabel::Right;

std::cout << pluss::series::print(index.resample_last(values, options)) << "\n";
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4.0000, 5.0000]
```

Example 6. Unsupported frequency text fails before output allocation.

```cpp
try {
  (void)index.resample_last(values, "fortnight");
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0,
      std::string_view(e.what()).find('\n')) << "\n";
}
```

### Output

```text
Timeline::TimelineIndexAccessor: unsupported freq (expected fixed units like S/min/H/D, weekly W, monthly M, yearly Y)
```

## Validation And Cost

The method rejects unordered indices, non-positive steps, unknown frequencies,
boolean payloads, non-CPU payloads, non-1D values, and length mismatch. It
allocates a fresh result and performs a stable forward scan, overwriting a
bin's candidate whenever a later valid lane is encountered.

## See Also

- [`Timeline::TimelineIndexAccessor::resample_first`](resample_first.md)
- [`Timeline::TimelineIndexAccessor::resample_max`](resample_max.md)
- [`Timeline::TimelineIndexAccessor::resample_ohlc`](resample_ohlc.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample_first.html">
    <span class="label">Previous</span>
    <span class="title">resample_first</span>
  </a>
  <a class="next-link" href="resample_max.html">
    <span class="label">Next</span>
    <span class="title">resample_max</span>
  </a>
</div>
