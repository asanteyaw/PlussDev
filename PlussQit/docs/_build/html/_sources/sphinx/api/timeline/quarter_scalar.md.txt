<!-- pluss-hand-authored -->

# Timeline::quarter_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::quarter_scalar() const
```

## Classify One Date Into A Calendar Quarter

`quarter_scalar()` returns `1..4` from the scalar instant's local month:
January-March is `1`, April-June `2`, July-September `3`, and October-December
`4`. These are ordinary calendar quarters beginning in January; the method
does not accept a fiscal-year offset.

The timezone matters near a month boundary because it determines the local
date first. Use [`quarter()`](quarter.md) for array-shaped values; this method
requires rank zero and returns a native C++ integer.

## Quarter Table

| Result | Months |
| ---: | --- |
| `1` | January through March |
| `2` | April through June |
| `3` | July through September |
| `4` | October through December |

## Example: Adjacent Dates Cross A Quarter

```cpp
const auto march = pluss::timeline::make("2024-03-31T12:00:00Z");
const auto april = pluss::timeline::make("2024-04-01T12:00:00Z");
std::cout << "March=" << march.quarter_scalar()
          << ", April=" << april.quarter_scalar() << '\n';
```

### Output

```text
March=1, April=2
```

## Example: Timezone Can Cross The Quarter Boundary

```cpp
const auto utc = pluss::timeline::make("2024-04-01T01:00:00Z");
const auto edmonton = utc.tz_convert("America/Edmonton");
std::cout << "UTC=" << utc.quarter_scalar()
          << ", Edmonton=" << edmonton.quarter_scalar() << '\n';
```

### Output

```text
UTC=2, Edmonton=1
```

## Example: Absence

```cpp
std::cout << pluss::timeline::make("").quarter_scalar() << '\n';
```

### Output

```text
-1
```

## Gotchas And Failure Modes

- Quarters are one-based and fixed to the January calendar year.
- `NaT` returns `-1`; rank-one and higher input throws.
- A timezone conversion can change the quarter without changing the instant.
- Use explicit month arithmetic for non-January fiscal calendars.

## See Also

- [`quarter()`](quarter.md): shape-preserving quarter extraction.
- [`month_scalar()`](month_scalar.md): the underlying local month.
- [`is_quarter_start()`](is_quarter_start.md): classify quarter boundaries.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html"><span class="label">Previous</span><span class="title">quarter</span></a>
  <a class="next-link" href="repeat.html"><span class="label">Next</span><span class="title">repeat</span></a>
</div>
