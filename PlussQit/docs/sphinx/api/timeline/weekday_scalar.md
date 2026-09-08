<!-- pluss-hand-authored -->

# Timeline::weekday_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::weekday_scalar() const
```

## Read One Weekday On A Stable Numeric Scale

`weekday_scalar()` maps the scalar instant's local calendar date to `0..6`,
with Monday equal to `0` and Sunday equal to `6`. This is the same convention
as [`weekday()`](weekday.md); moving between scalar and shape-preserving code
does not change the numbering.

The method answers a civil-calendar question. The clock time matters only when
the active timezone places the instant on a different local date. It does not
compute elapsed whole days and does not depend on locale names.

## Weekday Table

| Result | Day |
| ---: | --- |
| `0` | Monday |
| `1` | Tuesday |
| `2` | Wednesday |
| `3` | Thursday |
| `4` | Friday |
| `5` | Saturday |
| `6` | Sunday |

## Example: Leap Day

```cpp
const auto leap_day = pluss::timeline::make("2024-02-29T23:45:00Z");
std::cout << leap_day.weekday_scalar() << '\n';
```

### Output

```text
3
```

The result identifies Thursday under the Monday-zero table.

## Example: Week-End Boundary

```cpp
const auto friday = pluss::timeline::make("2021-01-01T12:00:00Z");
const auto sunday = pluss::timeline::make("2021-01-03T12:00:00Z");
std::cout << "Friday=" << friday.weekday_scalar()
          << ", Sunday=" << sunday.weekday_scalar() << '\n';
```

### Output

```text
Friday=4, Sunday=6
```

## Example: Absence And Shape

```cpp
const auto nat = pluss::timeline::make("");
std::cout << "absent=" << nat.weekday_scalar() << '\n';

try {
  static_cast<void>(
      pluss::timeline::make({"2021-01-01T12:00:00Z"}).weekday_scalar());
  std::cout << "accepted\n";
} catch (const std::exception&) {
  std::cout << "one-element vector rejected\n";
}
```

### Output

```text
absent=-1
one-element vector rejected
```

## Gotchas And Failure Modes

- Monday is `0`; do not reinterpret the result as a Sunday-zero scale.
- `NaT` returns `-1`, which is outside the weekday range.
- Exactly rank zero is required. Use `weekday()` for arrays.
- Names belong to `weekday_name()`; this method is numeric and locale-neutral.
- Convert timezone before extraction when local scheduling rules matter.

## See Also

- [`weekday()`](weekday.md): shape-preserving numeric extraction.
- [`weekday_name()`](weekday_name.md): textual labels.
- [`is_weekend()`](is_weekend.md): direct elementwise weekend classification.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html"><span class="label">Previous</span><span class="title">weekday_name</span></a>
  <a class="next-link" href="where.html"><span class="label">Next</span><span class="title">where</span></a>
</div>
