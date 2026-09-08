<!-- pluss-hand-authored -->

# Timeline::month_start_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::month_start_scalar() const
```

## Test The First Local Calendar Day

`month_start_scalar()` is `true` whenever the scalar instant's local day of
month is `1`. Clock time does not need to be midnight: every instant occurring
on the first local date satisfies the predicate.

This is a calendar-position query, not a test that the instant equals the
month's opening boundary. Use `floor("D")` if a midnight instant is required.
Use [`is_month_start()`](is_month_start.md) for arrays.

## Example: First Day Versus Last Day

```cpp
const auto first = pluss::timeline::make("2024-02-01T12:00:00Z");
const auto last = pluss::timeline::make("2024-02-29T12:00:00Z");
std::cout << std::boolalpha << "Feb 1=" << first.month_start_scalar()
          << ", Feb 29=" << last.month_start_scalar() << '\n';
```

### Output

```text
Feb 1=true, Feb 29=false
```

## Example: Timezone Can Change The Date

```cpp
const auto utc = pluss::timeline::make("2024-04-01T01:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << std::boolalpha << "UTC=" << utc.month_start_scalar()
          << ", Edmonton=" << local.month_start_scalar() << '\n';
```

### Output

```text
UTC=true, Edmonton=false
```

## Absence And Failure

`NaT` returns `false`. Any non-scalar shape throws, including shape `[1]`.
Neither result mutates or rounds the receiver.

## See Also

- [`is_month_start()`](is_month_start.md): shape-preserving predicate.
- [`month_end_scalar()`](month_end_scalar.md): test the month's final date.
- [`day_scalar()`](day_scalar.md): read the day-of-month field directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_scalar.html"><span class="label">Previous</span><span class="title">month_scalar</span></a>
  <a class="next-link" href="months_between.html"><span class="label">Next</span><span class="title">months_between</span></a>
</div>
