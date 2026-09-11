# Timeline::month

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::month() const
```

`month()` is the sole public spelling.

## Read The Month Number

`month()` returns the local calendar month for every timestamp. Months are
one-based: January is `1`, February is `2`, and December is `12`. The result is
an `int32` Series with the same shape as the Timeline; `NaT` maps to `-1`.

One-based numbering lets the values read naturally in reports, but it means a
month is not a zero-based array offset. Subtract one explicitly only when an
algorithm genuinely requires an index in `[0, 11]`.

## Example: Crossing Years Does Not Reset The Array

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.month());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[12, 1, 2, -1]
```

The output is a projection, not a grouping operation: December and January
remain in their original positions.

## Example: The Same Instant In Two Calendars

```cpp
const auto utc = pluss::timeline::make("2024-03-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");

std::cout << "UTC month=" << utc.month().item<std::int32_t>() << '\n';
std::cout << "New York month=" << local.month().item<std::int32_t>() << '\n';
```

### Output

```text
UTC month=3
New York month=2
```

`tz_convert()` did not move the instant. It changed the local date through
which that instant is interpreted, so the month changed at the boundary.

## Gotchas And Failure Modes

- The valid range is `1` through `12`; `0` is never January.
- `-1` identifies an absent instant.
- For a readable label such as `February`, use `month_name()`.
- For three-month buckets use `quarter()` rather than hand-written thresholds.

## See Also

- [`year`](year.md)
- [`day`](day.md)
- [`month_name`](month_name.md)
- [`quarter`](quarter.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minutes_between.html"><span class="label">Previous</span><span class="title">minutes_between</span></a>
  <a class="next-link" href="month_end_scalar.html"><span class="label">Next</span><span class="title">month_end_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
