<!-- pluss-hand-authored -->

# Timeline::month_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::month_scalar() const
```

## Read One Calendar Month

`month_scalar()` returns the scalar instant's local calendar month as an
ordinary integer from `1` through `12`. It is the scalar counterpart of
[`month()`](month.md): choose this method when C++ code needs one value, and
choose `month()` when rank and shape must be preserved in a `Series`.

Month extraction uses the Timeline's timezone. It does not divide epoch time
into fixed-duration blocks because calendar months have unequal lengths and
local midnight does not necessarily align with UTC midnight. A timezone
conversion near a month boundary can therefore change the result.

## Example: Leap-Day Month

```cpp
const auto leap_day = pluss::timeline::make("2024-02-29T23:45:00Z");
std::cout << "month=" << leap_day.month_scalar() << '\n';
```

### Output

```text
month=2
```

## Example: Timezone Crossing A Month Boundary

```cpp
const auto utc = pluss::timeline::make("2025-01-01T01:30:00Z");
const auto edmonton = utc.tz_convert("America/Edmonton");
std::cout << "UTC=" << utc.month_scalar() << '\n'
          << "Edmonton=" << edmonton.month_scalar() << '\n';
```

### Output

```text
UTC=1
Edmonton=12
```

## Example: Absence

```cpp
const auto nat = pluss::timeline::make("");
std::cout << nat.month_scalar() << '\n';
```

### Output

```text
-1
```

The absent sentinel lies outside the valid `1..12` range. A one-element vector
is not accepted; call `month()` for vector, matrix, or higher-rank values.

## Gotchas And Failure Modes

- The return value is one-based; January is `1`, not `0`.
- The receiver must be rank zero, not merely contain one element.
- `NaT` returns `-1` rather than throwing.
- Timezone conversion can change the month while preserving the instant.
- The operation is read-only and leaves all original precision intact.

## See Also

- [`month()`](month.md): shape-preserving extraction.
- [`month_name()`](month_name.md): textual month labels.
- [`quarter_scalar()`](quarter.md): the enclosing quarter as a scalar field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html"><span class="label">Previous</span><span class="title">month_name</span></a>
  <a class="next-link" href="month_start_scalar.html"><span class="label">Next</span><span class="title">month_start_scalar</span></a>
</div>
