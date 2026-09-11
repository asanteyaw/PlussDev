<!-- pluss-hand-authored -->

# Timeline::is_weekend_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::is_weekend_scalar() const
```

## Classify Saturday And Sunday

`is_weekend_scalar()` returns `true` when the scalar instant's local date is
Saturday or Sunday. It uses the ordinary seven-day week only. Public holidays,
regional work schedules, and custom weekmasks do not affect the result.

Use [`is_weekend()`](is_weekend.md) for array-shaped values. The scalar method
returns a native `bool` and requires rank zero; a one-element vector is not
silently squeezed.

## Example: Cross The Week-End Boundary

```cpp
const auto friday = pluss::timeline::make("2024-03-08T12:00:00Z");
const auto saturday = pluss::timeline::make("2024-03-09T12:00:00Z");
const auto sunday = pluss::timeline::make("2024-03-10T12:00:00Z");
std::cout << std::boolalpha << "Friday=" << friday.is_weekend_scalar()
          << ", Saturday=" << saturday.is_weekend_scalar()
          << ", Sunday=" << sunday.is_weekend_scalar() << '\n';
```

### Output

```text
Friday=false, Saturday=true, Sunday=true
```

## Example: Absence And Rank Validation

```cpp
std::cout << std::boolalpha
          << "absent=" << pluss::timeline::make("").is_weekend_scalar() << '\n';
try {
  (void)pluss::timeline::make({"2024-03-09"}).is_weekend_scalar();
} catch (const std::exception&) {
  std::cout << "one-element vector rejected\n";
}
```

### Output

```text
absent=false
one-element vector rejected
```

## Gotchas And Failure Modes

- The local timezone determines the weekday before classification.
- This method knows weekends, not business calendars or holidays.
- `NaT` returns `false`; check `isnat_scalar()` to preserve absence information.
- Non-scalar shapes throw and the receiver is never mutated.

## See Also

- [`is_weekend()`](is_weekend.md): shape-preserving predicate.
- [`weekday_scalar()`](weekday_scalar.md): inspect the Monday-zero weekday.
- [`is_business_day()`](is_business_day.md): elementwise weekday business rule.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_weekend.html"><span class="label">Previous</span><span class="title">is_weekend</span></a>
  <a class="next-link" href="is_year_end.html"><span class="label">Next</span><span class="title">is_year_end</span></a>
</div>
