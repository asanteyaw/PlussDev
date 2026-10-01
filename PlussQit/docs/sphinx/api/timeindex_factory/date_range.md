<!-- pluss-hand-authored -->

# pluss::timeindex::date_range

[Back to TimeIndex Factory API Reference](../../timeindex_factory_api_reference)

## Signature

```cpp
pluss::TimeIndex pluss::timeindex::date_range(
    const pluss::Timeline& start,
    const pluss::Timeline& end,
    std::string_view freq,
    std::int64_t step = 1,
    bool require_sorted = true)
```

## Build A Regular Temporal Axis

`date_range()` starts at scalar `start`, advances by `step` units of `freq`, and
includes `end` when repeated stepping lands on it exactly. It returns a
one-dimensional TimeIndex directly, avoiding a temporary public Timeline at
the call site.

Fixed units such as seconds, minutes, and hours advance elapsed nanoseconds.
Day frequency advances civil calendar days in `start`'s timezone, preserving
the local clock across offset changes. The result inherits the start timezone;
the end value is compared as an instant.

`step` may have magnitude greater than one. Positive stepping is naturally
non-decreasing and works with the default ordering check. Negative stepping
constructs a descending range and therefore requires `require_sorted=false`.
Zero is rejected because it cannot make progress. When step direction points
away from the endpoint, the result is an empty TimeIndex.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `start` | Inclusive scalar first instant and source timezone | required |
| `end` | Inclusive scalar bound | required |
| `freq` | Step unit such as `"S"`, `"min"`, `"H"`, or `"D"` | required |
| `step` | Non-zero signed count of frequency units | `1` |
| `require_sorted` | Require non-decreasing output before returning | `true` |

Both endpoints must be defined scalar Timelines. `NaT`, non-scalar endpoints,
unsupported frequencies, arithmetic overflow, and zero steps fail before a
partial result is returned.

## Examples

Example 1. Build an inclusive two-hour cadence.

```cpp
auto start = pluss::timeline::make("2025-01-01T00:00:00Z");
auto end = pluss::timeline::make("2025-01-01T06:00:00Z");
auto index = pluss::timeindex::date_range(start, end, "H", 2);
std::cout << pluss::timeindex::print(index) << "\n";
```

### Output

```text
TimeIndex(shape=[4], tz=UTC)
[ 2025-01-01 00:00:00, 2025-01-01 02:00:00, 2025-01-01 04:00:00, 2025-01-01 06:00:00 ]
```

Example 2. Civil days retain the local hour across the spring offset change.

```cpp
auto start = pluss::timeline::make("2024-03-09T09:00:00")
                 .tz_localize("America/Edmonton");
auto end = pluss::timeline::make("2024-03-11T09:00:00")
               .tz_localize("America/Edmonton");
std::cout << pluss::timeindex::print(
    pluss::timeindex::date_range(start, end, "D")) << "\n";
```

### Output

```text
TimeIndex(shape=[3], tz=America/Edmonton)
[ 2024-03-09 09:00:00, 2024-03-10 09:00:00, 2024-03-11 09:00:00 ]
```

Example 3. Negative stepping is available when descending index semantics are
requested explicitly.

```cpp
auto descending =
    pluss::timeindex::date_range(end, start, "D", -1, false);
std::cout << pluss::timeindex::print(descending) << "\n";
```

### Output

```text
TimeIndex(shape=[3], tz=America/Edmonton)
[ 2024-03-11 09:00:00, 2024-03-10 09:00:00, 2024-03-09 09:00:00 ]
```

Example 4. Leaving the default ordering check enabled rejects that descending
axis.

```cpp
try {
  (void)pluss::timeindex::date_range(end, start, "D", -1);
} catch (const std::exception& error) {
  std::cout << std::string_view(error.what()).substr(0,
      std::string_view(error.what()).find('\n')) << "\n";
}
```

### Output

```text
TimeIndex must be sorted (non-decreasing)
```

Example 5. A zero step is invalid.

```cpp
try {
  (void)pluss::timeindex::date_range(start, end, "D", 0);
} catch (const std::exception& error) {
  std::cout << std::string_view(error.what()).substr(0,
      std::string_view(error.what()).find('\n')) << "\n";
}
```

### Output

```text
date_range step must be non-zero
```

## Cost And Precision

Each output tick is generated exactly in signed 64-bit nanoseconds. The
operation runs in `O(result.size())` time and allocates one packed tick buffer.
Civil-day stepping performs timezone conversion per lane; fixed-unit stepping
uses checked nanosecond arithmetic.

## See Also

- [`make`](make.md)
- [`TimelineIndexAccessor::freq`](../timeline-index/freq.md)
- [`TimelineIndexAccessor::is_monotonic_decreasing`](../timeline-index/is_monotonic_decreasing.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../timeindex_factory_api_reference.html"><span class="label">Previous</span><span class="title">TimeIndex Factory API Reference</span></a>
  <a class="next-link" href="from_buffer.html"><span class="label">Next</span><span class="title">from_buffer</span></a>
</div>
