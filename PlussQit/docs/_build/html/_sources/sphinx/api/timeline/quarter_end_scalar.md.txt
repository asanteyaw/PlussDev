<!-- pluss-hand-authored -->

# Timeline::quarter_end_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::quarter_end_scalar() const
```

## Test A Standard Quarter's Final Date

`quarter_end_scalar()` is `true` on the final local dates of March, June,
September, and December. It checks both the quarter-ending month and that
month's actual last day. Any time of day qualifies.

This is not a fiscal-calendar operation and not an exact boundary-instant
test. Use [`is_quarter_end()`](is_quarter_end.md) for array-shaped input.

## Example: End Then Start

```cpp
const auto march = pluss::timeline::make("2024-03-31T12:00:00Z");
const auto april = pluss::timeline::make("2024-04-01T12:00:00Z");
std::cout << std::boolalpha << "Mar 31=" << march.quarter_end_scalar()
          << ", Apr 1=" << april.quarter_end_scalar() << '\n';
```

### Output

```text
Mar 31=true, Apr 1=false
```

## Example: UTC Start, Local End

```cpp
const auto utc = pluss::timeline::make("2024-04-01T01:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << std::boolalpha << "UTC=" << utc.quarter_end_scalar()
          << ", Edmonton=" << local.quarter_end_scalar() << '\n';
```

### Output

```text
UTC=false, Edmonton=true
```

## Gotchas And Failure Modes

- Quarter ends are calendar dates, not fixed elapsed intervals.
- The active timezone can change the classified date.
- `NaT` returns `false`; rank-one and higher input throws.
- Standard quarters are fixed to the January-based year.

## See Also

- [`is_quarter_end()`](is_quarter_end.md): array predicate.
- [`quarter_start_scalar()`](quarter_start_scalar.md): opening date.
- [`month_end_scalar()`](month_end_scalar.md): month-end classification.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html"><span class="label">Previous</span><span class="title">quarter</span></a>
  <a class="next-link" href="quarter_scalar.html"><span class="label">Next</span><span class="title">quarter_scalar</span></a>
</div>
