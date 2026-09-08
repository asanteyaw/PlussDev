# Timeline::hours_between

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::hours_between(const Timeline& other) const
```

## Count Complete Elapsed Hours

`hours_between()` returns the signed number of complete 3,600-second units from
one scalar instant to another. It compares UTC epoch nanoseconds, so timezone
labels and local-clock discontinuities do not alter the duration. Any
incomplete hour is discarded by truncation toward zero.

Use it when the application asks how many whole elapsed hours have passed. Do
not derive that answer by comparing extracted [`hour()`](hour.md) fields: those
fields repeat every day and can repeat or disappear at timezone transitions.

## Example: Preserve More Detail Than Whole Days

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto end = pluss::timeline::make("2024-01-02T02:03:04.500000000Z");

std::cout << "days=" << start.days_between(end) << '\n'
          << "hours=" << start.hours_between(end) << '\n';
```

### Output

```text
days=1
hours=26
```

The same interval contains one complete 24-hour unit but 26 complete hours.
The remaining minutes and seconds do not produce a 27th hour.

## Example: Daylight-Saving Changes Do Not Invent Elapsed Time

```cpp
const auto before = pluss::timeline::make("2024-03-09T10:30:00Z")
                        .tz_convert("America/Edmonton");
const auto next_civil_day = before.add_days(1);

std::cout << before.hours_between(next_civil_day) << '\n';
```

### Output

```text
23
```

The values share the same displayed clock on consecutive dates, but the
offset transition makes their absolute separation 23 hours. `hours_between()`
reports that absolute separation.

## Example: Presentation Zones May Differ

```cpp
const auto utc = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto same = utc.tz_convert("America/Edmonton");

std::cout << utc.hours_between(same) << '\n';
```

### Output

```text
0
```

Timezone conversion changes the wall-clock view, not the instant. Both
operands must still be scalar and neither may be `NaT`; violations throw.
The method is constant-time, performs overflow-safe widened subtraction, and
does not allocate or mutate either operand.

## See Also

- [`days_between`](days_between.md): count complete 24-hour units.
- [`minutes_between`](minutes_between.md): retain completed minutes.
- [`add_hours`](add_hours.md): construct an elapsed-hour offset.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour_scalar.html"><span class="label">Previous</span><span class="title">hour_scalar</span></a>
  <a class="next-link" href="is_business_day.html"><span class="label">Next</span><span class="title">is_business_day</span></a>
</div>
<!-- pluss-hand-authored -->
