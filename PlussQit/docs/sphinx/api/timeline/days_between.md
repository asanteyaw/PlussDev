# Timeline::days_between

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::days_between(const Timeline& other) const
```

## Count Complete Elapsed 24-Hour Units

`days_between()` subtracts two scalar instants and returns the signed number of
complete 24-hour units from the receiver to `other`. One day is exactly
86,400,000,000,000 nanoseconds. The quotient truncates toward zero, so an
interval of 47 hours returns `1`, while `-47` hours returns `-1`.

This is deliberately different from [`add_days()`](add_days.md), which moves
to the same local clock on another calendar date. A civil day can contain 23,
24, or 25 elapsed hours when a timezone offset changes. Use `days_between()`
for elapsed-duration thresholds; use calendar operations when date labels are
the governing concept.

## Example: Incomplete Units Are Discarded

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto end = pluss::timeline::make("2024-01-02T02:03:04.500000000Z");

std::cout << start.days_between(end) << '\n'
          << end.days_between(start) << '\n';
```

### Output

```text
1
-1
```

The remaining two hours, three minutes, four seconds, and 500 milliseconds do
not round the answer away from zero. Reverse order changes only the sign.

## Example: A Civil Day Across Spring DST Is Only 23 Hours

```cpp
const auto before = pluss::timeline::make("2024-03-09T10:30:00Z")
                        .tz_convert("America/Edmonton");
const auto next_civil_day = before.add_days(1);

std::cout << "hours=" << before.hours_between(next_civil_day) << '\n'
          << "days=" << before.days_between(next_civil_day) << '\n'
          << "after_24_hours="
          << before.days_between(before.add_hours(24)) << '\n';
```

### Output

```text
hours=23
days=0
after_24_hours=1
```

The first endpoint is the next local date at the same clock time, but the
spring transition removes one elapsed hour. It therefore has not completed an
elapsed 24-hour unit.

## Example: Non-Scalar And `NaT` Inputs Are Rejected

```cpp
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");

for (const auto& other : {
         pluss::timeline::make(""),
         pluss::timeline::make({"2024-01-01", "2024-01-02"}),
     }) {
  try {
    (void)start.days_between(other);
  } catch (const pluss::Error&) {
    std::cout << "rejected\n";
  }
}
```

### Output

```text
rejected
rejected
```

Both operands must be scalar defined instants. Timezone names need not match
for elapsed methods because epoch subtraction is independent of presentation
zone. The implementation uses widened subtraction, so opposite ends of the
valid epoch-nanosecond range cannot overflow before division.

## See Also

- [`hours_between`](hours_between.md): retain completed elapsed hours.
- [`add_days`](add_days.md): move by local calendar days.
- [`span`](span.md): compute shape-preserving elementwise day spans.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dayofyear.html"><span class="label">Previous</span><span class="title">dayofyear</span></a>
  <a class="next-link" href="defined.html"><span class="label">Next</span><span class="title">defined</span></a>
</div>
<!-- pluss-hand-authored -->
