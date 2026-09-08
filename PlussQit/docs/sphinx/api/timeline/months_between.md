# Timeline::months_between

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
std::int64_t Timeline::months_between(const Timeline& other) const
```

## Count Complete Calendar-Month Movements

`months_between()` answers how many whole calendar-month movements fit between
two scalar instants. Read `a.months_between(b)` as "the signed number of
complete months from `a` to `b`." It returns a native integer, not a
one-element `Series`, and is intended for scalar control flow and interval
classification.

This is calendar arithmetic in the shared timezone. It is not elapsed duration
divided by an average month length; months have no fixed nanosecond size. The
method first estimates the calendar-month separation, moves the earlier instant
by that many months using the same clamping rule as [`add_months()`](add_months.md),
and counts the final month only if that anniversary has been reached.

Both operands must be scalar, defined instants with identical `tz()` names.
The result is antisymmetric: reversing the operands reverses the sign without
changing the magnitude.

## Example: End-Of-Month Clamping Defines The Anniversary

```cpp
const auto start = pluss::timeline::make("2023-01-31T10:00:00Z");
const auto anniversary = pluss::timeline::make("2023-02-28T10:00:00Z");
const auto one_second_early =
    pluss::timeline::make("2023-02-28T09:59:59Z");

std::cout << "complete=" << start.months_between(anniversary) << '\n'
          << "incomplete=" << start.months_between(one_second_early) << '\n';
```

### Output

```text
complete=1
incomplete=0
```

Adding one month to 31 January clamps to the last valid day of February while
preserving 10:00. Consequently, 28 February at 10:00 completes one month;
09:59:59 does not. This rule also handles 29, 30, and 31-day month boundaries
without introducing an arbitrary average month.

## Example: Direction Is Encoded In The Sign

```cpp
const auto january = pluss::timeline::make("2023-01-31T10:00:00Z");
const auto february = pluss::timeline::make("2023-02-28T10:00:00Z");

std::cout << january.months_between(february) << '\n'
          << february.months_between(january) << '\n';
```

### Output

```text
1
-1
```

Intervals shorter than one complete month return zero in either direction.
There is no negative zero.

## Example: Calendar Zones Must Match

```cpp
const auto utc = pluss::timeline::make("2024-01-01T00:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");

try {
  (void)utc.months_between(local);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

The two values represent the same instant, but their displayed calendars use
different zones. A calendar-month answer would therefore have no single civil
frame of reference. Convert one operand explicitly before calling.

## Failures And Cost

Vectors, matrices, `NaT`, or mismatched timezone names throw before a result is
returned. The method performs constant work and allocates only the temporary
scalar anniversary needed to apply calendar clamping. For elementwise or
broadcasted distances, use [`span()`](span.md) with
`TimelineSpanUnit::Months`; that method returns a shape-preserving `Series` and
offers boundary, completion, anchor, and sign policies.

## See Also

- [`add_months`](add_months.md): construct a calendar-month anniversary.
- [`years_between`](years_between.md): count complete calendar years.
- [`span`](span.md): calculate configurable elementwise spans.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_scalar.html"><span class="label">Previous</span><span class="title">month_scalar</span></a>
  <a class="next-link" href="nanosecond.html"><span class="label">Next</span><span class="title">nanosecond</span></a>
</div>
<!-- pluss-hand-authored -->
