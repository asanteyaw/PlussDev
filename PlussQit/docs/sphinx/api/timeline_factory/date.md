<!-- pluss-hand-authored -->
# pluss::timeline::date

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::date(std::string_view value)
Timeline pluss::timeline::date(string_range values)
Timeline pluss::timeline::date(nested_string_values values)
Timeline pluss::timeline::date(string_range flat, integer_range sizes)
```

## Require Calendar Dates

`date()` parses date-only text and anchors every defined value at midnight UTC.
It exists to make a calendar-date requirement executable: a clock or timezone
suffix is rejected rather than ignored. Use it when accepting a complete
timestamp would conceal malformed input; use `make()` when clock fields are
valid data.

The result is still a `Timeline`, not a date-without-time type. Each lane is a
real instant at `00:00:00.000000000+00:00`, so comparison, duration,
formatting, and timezone operations remain available. Converting that instant
to a timezone west of UTC can display the preceding local date. If the desired
midnight belongs to a local civil timezone, apply explicit localization rather
than assuming that `date()` guesses a local zone.

Accepted tokens use `YYYY-MM-DD` or `YYYY/MM/DD`, with optional surrounding
ASCII whitespace. Empty text is not accepted here: a strict date lane must
actually contain a date.

## Example 1: Scalar Date

```cpp
const auto leap_day = pluss::timeline::date("2000-02-29");
std::cout << pluss::note::print(
    pluss::timeline::format(leap_day, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2000-02-29T00:00:00.000000000+00:00"
```

Scalar text produces shape `[]` and one element.

## Example 2: A Range Produces One Dimension

```cpp
const auto days = pluss::timeline::date({"2024-02-29", "2024/03/01"});
std::cout << pluss::note::print(
    pluss::timeline::format(days, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-02-29T00:00:00.000000000+00:00", "2024-03-01T00:00:00.000000000+00:00"]
```

## Example 3: Nested Calendar Geometry

```cpp
const auto calendar = pluss::timeline::date({
    {"2024-01-01", "2024-01-02"},
    {"2024-02-01", "2024-02-02"},
});
std::cout << pluss::note::print(
    pluss::timeline::format(calendar, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-01T00:00:00.000000000+00:00", "2024-01-02T00:00:00.000000000+00:00"], 
  ["2024-02-01T00:00:00.000000000+00:00", "2024-02-02T00:00:00.000000000+00:00"]
]
```

Nested rows must be rectangular. Their inferred rank and shape are retained.

## Example 4: Flat Runtime Data And Explicit Shape

```cpp
const std::vector<std::string_view> flat{
    "2024-05-01", "2024-05-02", "2024-05-03", "2024-05-04"};
const auto matrix = pluss::timeline::date(flat, {2, 2});
std::cout << pluss::note::print(
    pluss::timeline::format(matrix, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-05-01T00:00:00.000000000+00:00", "2024-05-02T00:00:00.000000000+00:00"], 
  ["2024-05-03T00:00:00.000000000+00:00", "2024-05-04T00:00:00.000000000+00:00"]
]
```

Use this overload when shape is runtime metadata rather than C++ nesting.

## Example 5: Empty Geometry Is Still Defined

```cpp
const auto empty = pluss::timeline::date(
    std::vector<std::string_view>{}, {2, 0});
std::cout << pluss::timeline::print(empty) << '\n';
```

### Output

```text
Timeline(shape=[2, 0], tz=UTC)[  ]
[  ]
```

The shape is validated even though there are no tokens to parse.

## Validation And Failure

`date()` rejects impossible dates, clocks, offsets, empty tokens, ragged
nesting, negative extents, ranks above three, and explicit shapes whose element
count differs from the flat range.

```cpp
try {
  (void)pluss::timeline::date("2024-06-10T09:30:00Z");
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

No truncation occurs: the clock makes the token invalid for this factory.

## See Also

- [`make`](make.md): accept full timestamp text.
- [`time`](time.md): construct epoch-anchored clock readings.
- [`date_range`](date_range.md): generate regular calendar-day sequences.
