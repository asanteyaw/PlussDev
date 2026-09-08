<!-- pluss-hand-authored -->

# Timeline::month_end_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::month_end_scalar() const
```

## Test The Actual Final Day Of A Month

`month_end_scalar()` compares the scalar instant's local date with the true
length of its month. It therefore recognizes February 28 in common years,
February 29 in leap years, and the correct 30- or 31-day endings elsewhere.
Any clock time on that final local date qualifies; midnight is not required.

Use [`is_month_end()`](is_month_end.md) for array-shaped input. The scalar form
returns `bool` and rejects nonzero rank rather than selecting an element.

## Example: Leap-February Boundary

```cpp
const auto first = pluss::timeline::make("2024-02-01T12:00:00Z");
const auto last = pluss::timeline::make("2024-02-29T12:00:00Z");
std::cout << std::boolalpha << "Feb 1=" << first.month_end_scalar()
          << ", Feb 29=" << last.month_end_scalar() << '\n';
```

### Output

```text
Feb 1=false, Feb 29=true
```

## Example: Same Instant Across A Month Boundary

```cpp
const auto utc = pluss::timeline::make("2024-04-01T01:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << std::boolalpha << "UTC=" << utc.month_end_scalar()
          << ", Edmonton=" << local.month_end_scalar() << '\n';
```

### Output

```text
UTC=false, Edmonton=true
```

The Edmonton view is still March 31.

## Gotchas And Failure Modes

- The predicate tests the whole local date, not an exact midnight boundary.
- Month length is calendar-derived; it is never assumed to be 30 days.
- `NaT` returns `false`; rank-one and higher input throws.
- Timezone conversion may flip the answer without changing the instant.

## See Also

- [`is_month_end()`](is_month_end.md): shape-preserving predicate.
- [`month_start_scalar()`](month_start_scalar.md): test the first local date.
- [`leap_scalar()`](leap_scalar.md): classify February's year.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html"><span class="label">Previous</span><span class="title">month</span></a>
  <a class="next-link" href="month_name.html"><span class="label">Next</span><span class="title">month_name</span></a>
</div>
