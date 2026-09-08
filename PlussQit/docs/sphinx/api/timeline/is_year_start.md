# Timeline::is_year_start

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_year_start() const
```

`is_year_start()` is the sole public spelling.

## Recognize January 1 In Local Time

`is_year_start()` returns `true` whenever a timestamp's local month and day are
January 1. The time-of-day does not matter. The result is a shape-preserving
boolean Series, and `NaT` maps to `false`.

## Example: Year Starts In A Mixed Calendar

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_year_start());
```

### Output

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, false, false, false, false, true, false, false]
```

Only 1 January 2020 and 1 January 2021 qualify. Leap-year status has no effect
on where a civil year begins.

## Example: One Instant, Two Local Dates

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC year_start=" << utc.is_year_start().item<bool>() << '\n'
          << "New York year_start=" << local.is_year_start().item<bool>() << '\n';
```

### Output

```text
UTC year_start=true
New York year_start=false
```

The instant is unchanged; only its local calendar representation changed.

## Gotchas And Failure Modes

- This means civil January 1, not fiscal-year start.
- It marks every timestamp on that date, not only midnight.
- Convert to the reporting timezone before creating the mask.

## See Also

- [`is_year_end`](is_year_end.md)
- [`is_quarter_start`](is_quarter_start.md)
- [`year`](year.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_quarter_end.html"><span class="label">Previous</span><span class="title">is_quarter_end</span></a>
  <a class="next-link" href="is_year_end.html"><span class="label">Next</span><span class="title">is_year_end</span></a>
</div>
<!-- pluss-hand-authored -->
