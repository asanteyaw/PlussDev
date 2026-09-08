# Timeline::hour

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::hour() const
```

`hour()` is the sole public spelling.

## Read The Local Hour

`hour()` returns the hour-of-day using a 24-hour clock. Valid values are `0`
through `23`: midnight begins at `0`, noon begins at `12`, and the final hour
of a day is `23`. The result is shape-preserving `int32`; `NaT` becomes `-1`.

This is a wall-clock field, not elapsed time. Two timestamps with the same
`hour()` may be days or years apart, and timezone conversion can change the
hour while preserving the underlying instant.

## Example: Midnight And Late Evening

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.hour());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[23, 0, 6, -1]
```

## Example: Convert Before Building Local-Hour Features

```cpp
const auto utc = pluss::timeline::make("2024-03-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");

std::cout << "UTC hour=" << utc.hour().item<std::int32_t>() << '\n';
std::cout << "New York hour=" << local.hour().item<std::int32_t>() << '\n';
```

### Output

```text
UTC hour=0
New York hour=19
```

For business-hour filters, convert to the business timezone first. Extracting
in UTC and then relabeling the result cannot recover the local hour.

## Gotchas And Failure Modes

- The clock is 24-hour; there is no AM/PM return value.
- `hour()` is not a duration and cannot measure elapsed hours.
- Repeated or skipped local hours around offset transitions are valid calendar
  behavior; include the date and timezone when identity matters.

## See Also

- [`minute`](minute.md)
- [`date`](date.md)
- [`tz_convert`](tz_convert.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html"><span class="label">Previous</span><span class="title">day</span></a>
  <a class="next-link" href="hour_scalar.html"><span class="label">Next</span><span class="title">hour_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
