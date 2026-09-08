<!-- pluss-hand-authored -->

# Timeline::quarter_start_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::quarter_start_scalar() const
```

## Test A Standard Quarter's First Date

`quarter_start_scalar()` is `true` on January 1, April 1, July 1, and October
1 in the scalar instant's active timezone. Every clock time on those dates
qualifies. The quarters are standard January-based calendar quarters; there is
no fiscal-year offset parameter.

Use [`is_quarter_start()`](is_quarter_start.md) for a shape-preserving boolean
`Series`. This method requires rank zero and returns one `bool`.

## Example: Cross The Q1/Q2 Boundary

```cpp
const auto march = pluss::timeline::make("2024-03-31T12:00:00Z");
const auto april = pluss::timeline::make("2024-04-01T12:00:00Z");
std::cout << std::boolalpha << "Mar 31=" << march.quarter_start_scalar()
          << ", Apr 1=" << april.quarter_start_scalar() << '\n';
```

### Output

```text
Mar 31=false, Apr 1=true
```

## Example: Timezone Reclassifies The Same Instant

```cpp
const auto utc = pluss::timeline::make("2024-04-01T01:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << std::boolalpha << "UTC=" << utc.quarter_start_scalar()
          << ", Edmonton=" << local.quarter_start_scalar() << '\n';
```

### Output

```text
UTC=true, Edmonton=false
```

## Contract Summary

- Only the four standard quarter-opening dates return `true`.
- Clock time within the date is irrelevant.
- `NaT` returns `false`; non-scalar shape throws.
- The method is timezone-aware and read-only.

## See Also

- [`is_quarter_start()`](is_quarter_start.md): array predicate.
- [`quarter_end_scalar()`](quarter_end_scalar.md): final quarter date.
- [`quarter_scalar()`](quarter_scalar.md): enclosing quarter number.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter_scalar.html"><span class="label">Previous</span><span class="title">quarter_scalar</span></a>
  <a class="next-link" href="repeat.html"><span class="label">Next</span><span class="title">repeat</span></a>
</div>
