<!-- pluss-hand-authored -->

# Timeline::leap_scalar

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::leap_scalar() const
```

## Classify One Local Calendar Year

`leap_scalar()` reports whether a scalar instant's local civil year contains
February 29. It applies the Gregorian rule: years divisible by four are leap
years, except century years not divisible by 400.

The timezone is significant near New Year because it selects the local year
before classification. Use [`is_leap_year()`](is_leap_year.md) for an
array-shaped Timeline; that method preserves shape in a boolean `Series`, while
`leap_scalar()` requires rank zero and returns an ordinary `bool`.

## Example: Leap And Common Years

```cpp
const auto leap = pluss::timeline::make("2024-02-29T23:45:00Z");
const auto common = pluss::timeline::make("2023-02-28T12:00:00Z");
std::cout << std::boolalpha << "2024=" << leap.leap_scalar()
          << ", 2023=" << common.leap_scalar() << '\n';
```

### Output

```text
2024=true, 2023=false
```

## Example: One Instant, Two Local Years

```cpp
const auto utc = pluss::timeline::make("2024-01-01T01:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << std::boolalpha << "UTC=" << utc.leap_scalar()
          << ", Edmonton=" << local.leap_scalar() << '\n';
```

### Output

```text
UTC=true, Edmonton=false
```

UTC sees 2024; Edmonton still sees 2023. The represented instant is unchanged.

## Example: Absence And Rank

```cpp
std::cout << std::boolalpha
          << "absent=" << pluss::timeline::make("").leap_scalar() << '\n';
try {
  (void)pluss::timeline::make({"2024-01-01"}).leap_scalar();
} catch (const std::exception&) {
  std::cout << "one-element vector rejected\n";
}
```

### Output

```text
absent=false
one-element vector rejected
```

`false` for `NaT` means the predicate is not satisfied; test
`isnat_scalar()` when absence must be distinguished from a common year.

## See Also

- [`is_leap_year()`](is_leap_year.md): shape-preserving classification.
- [`year_scalar()`](year_scalar.md): inspect the classified civil year.
- [`month_end_scalar()`](month_end_scalar.md): respects February's actual length.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html"><span class="label">Previous</span><span class="title">le</span></a>
  <a class="next-link" href="lt.html"><span class="label">Next</span><span class="title">lt</span></a>
</div>
