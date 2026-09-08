# Timeline::years_between

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::years_between(const Timeline& other) const
```

## Count Completed Calendar-Year Anniversaries

`years_between()` returns the signed number of complete calendar years from
the scalar receiver to scalar `other`. It is an anniversary calculation, not
elapsed seconds divided by 365 days. The date, local clock, and timezone all
participate in deciding whether the final anniversary has occurred.

The method uses the same calendar clamping as [`add_years()`](add_years.md).
That matters for a leap-day receiver: its anniversary in a non-leap year is 28
February at the same local time. The result is a native `std::int64_t`, making
the method appropriate for scalar age, tenure, and completed-cycle logic.

## Example: A Leap-Day Anniversary Clamps To February 28

```cpp
const auto leap = pluss::timeline::make("2024-02-29T12:00:00Z");
const auto anniversary = pluss::timeline::make("2025-02-28T12:00:00Z");
const auto one_second_early =
    pluss::timeline::make("2025-02-28T11:59:59Z");

std::cout << "complete=" << leap.years_between(anniversary) << '\n'
          << "incomplete=" << leap.years_between(one_second_early) << '\n';
```

### Output

```text
complete=1
incomplete=0
```

The date alone is insufficient. At 11:59:59 the clamped anniversary remains
one second away, so no complete year is counted.

## Example: Reverse Intervals Are Symmetric

```cpp
const auto start = pluss::timeline::make("2024-02-29T12:00:00Z");
const auto end = pluss::timeline::make("2025-02-28T12:00:00Z");

std::cout << start.years_between(end) << '\n'
          << end.years_between(start) << '\n';
```

### Output

```text
1
-1
```

The method computes the complete-unit magnitude chronologically and applies
the direction afterward. This avoids the asymmetric floor behavior that would
turn an incomplete reverse interval into an extra negative year.

## Example: Reject An Ambiguous Calendar Frame

```cpp
const auto utc = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");

try {
  (void)utc.years_between(local);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

Calendar distances require matching timezone names even when two values denote
the same instant. Explicit conversion makes the intended local calendar clear.
Both operands must also be scalar and neither may be `NaT`.

## Choosing The Related Operation

Use `years_between()` for one signed scalar count under clamped-anniversary
semantics. Use [`add_years()`](add_years.md) to construct the anniversary. Use
[`span()`](span.md) for arrays, broadcasting, unsigned magnitudes, boundary
counts, or explicit month-end anchoring. The method is constant-time and does
not mutate either operand.

## See Also

- [`months_between`](months_between.md): count complete calendar months.
- [`add_years`](add_years.md): move by calendar years.
- [`span`](span.md): configure elementwise calendar spans.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="yearday_scalar.html"><span class="label">Previous</span><span class="title">yearday_scalar</span></a>
  <a class="next-link" href="../../timeline_api_reference.html"><span class="label">Next</span><span class="title">Timeline API Reference</span></a>
</div>
<!-- pluss-hand-authored -->
