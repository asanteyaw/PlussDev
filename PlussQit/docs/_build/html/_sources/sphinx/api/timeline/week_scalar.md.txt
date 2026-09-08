<!-- pluss-hand-authored -->

# Timeline::week_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
int Timeline::week_scalar() const
```

## Read One ISO Week Number

`week_scalar()` returns the local date's ISO week number, `1..53`. ISO weeks
begin on Monday, and week `1` is the week containing January 4. Consequently,
the first days of January can belong to week `52` or `53`, while the final days
of December can belong to week `1` of the following ISO week-year.

The method returns only the week number. Do not pair it blindly with
`year_scalar()` as a unique ISO year-week key: the two calendars disagree at
some New Year boundaries. It is appropriate when the week number itself is
needed; a complete ISO week identity also requires the corresponding ISO
week-year.

Use [`week()`](week.md) for shape-preserving array extraction.

## Example: January Can Still Be Week 53

```cpp
const auto friday = pluss::timeline::make("2021-01-01T12:00:00Z");
const auto monday = pluss::timeline::make("2021-01-04T12:00:00Z");
std::cout << "2021-01-01=" << friday.week_scalar()
          << ", 2021-01-04=" << monday.week_scalar() << '\n';
```

### Output

```text
2021-01-01=53, 2021-01-04=1
```

## Example: Civil Year And ISO Week Can Diverge

```cpp
const auto t = pluss::timeline::make("2024-12-31T12:00:00Z");
std::cout << "year=" << t.year_scalar()
          << ", week=" << t.week_scalar() << '\n';
```

### Output

```text
year=2024, week=1
```

The `1` belongs to the next ISO week-year even though the civil year field is
still `2024`.

## Example: Absence And Rank Validation

```cpp
std::cout << "absent=" << pluss::timeline::make("").week_scalar() << '\n';
try {
  (void)pluss::timeline::make({"2021-01-04T12:00:00Z"}).week_scalar();
} catch (const std::exception&) {
  std::cout << "one-element vector rejected\n";
}
```

### Output

```text
absent=-1
one-element vector rejected
```

## Contract Summary

- Valid output is `1..53`; `NaT` returns `-1`.
- Weeks begin Monday and follow ISO New Year rules.
- Rank zero is mandatory; no implicit first-element extraction occurs.
- The active timezone determines the local date before classification.
- The operation does not mutate or round the Timeline.

## See Also

- [`week()`](week.md): shape-preserving ISO week extraction.
- [`weekday_scalar()`](weekday_scalar.md): Monday-zero weekday.
- [`year_scalar()`](year_scalar.md): civil year, not necessarily ISO week-year.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html"><span class="label">Previous</span><span class="title">week</span></a>
  <a class="next-link" href="weekday.html"><span class="label">Next</span><span class="title">weekday</span></a>
</div>
