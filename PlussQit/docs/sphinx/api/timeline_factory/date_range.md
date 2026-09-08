<!-- pluss-hand-authored -->
# pluss::timeline::date_range

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::date_range(const Timeline& start,
                                     const Timeline& end,
                                     std::string_view frequency,
                                     std::int64_t step = 1)
```

## Build A Regular Inclusive Sequence

`date_range()` starts at one scalar instant and repeatedly advances by
`step * frequency` while values remain within the inclusive endpoint. It
always returns a rank-one `Timeline`, including when no values satisfy the
requested direction.

Use it when the unit is part of the schedule: seconds, minutes, hours, or local
calendar days. Use `arange()` when the step already exists as an exact
`Duration`, or when the upper boundary must be excluded.

The supported frequency spellings are:

- seconds: `"S"`, `"s"`, `"sec"`, `"second"`
- minutes: `"min"`, `"m"`, `"minute"`
- hours: `"H"`, `"h"`, `"hour"`
- calendar days: `"D"`, `"d"`, `"day"`

Seconds, minutes, and hours are elapsed-time frequencies. A day is a local
calendar frequency under `start.tz()`: it preserves the displayed clock across
offset transitions and can therefore span 23 or 25 elapsed hours. The result
retains `start.tz()`. `end` supplies an instant bound; its display timezone does
not replace the result timezone.

## Endpoint And Direction Rules

`start` is always included when direction is viable. `end` is included only if
repeated stepping lands on it exactly. A positive step with `start > end`, or a
negative step with `start < end`, returns a defined empty vector. The function
does not reverse a mistaken sign automatically.

## Example 1: Default One-Second Step

```cpp
const auto out = pluss::timeline::date_range(
    pluss::timeline::make("2024-01-01T00:00:00Z"),
    pluss::timeline::make("2024-01-01T00:00:03Z"), "S");
std::cout << pluss::note::print(
    pluss::timeline::format(out, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-01-01T00:00:00.000000000+00:00", "2024-01-01T00:00:01.000000000+00:00", "2024-01-01T00:00:02.000000000+00:00", "2024-01-01T00:00:03.000000000+00:00"]
```

## Example 2: A Step That Does Not Land On `end`

```cpp
const auto out = pluss::timeline::date_range(
    pluss::timeline::date("2024-01-01"),
    pluss::timeline::date("2024-01-06"), "D", 2);
std::cout << pluss::note::print(
    pluss::timeline::format(out, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-01-01T00:00:00.000000000+00:00", "2024-01-03T00:00:00.000000000+00:00", "2024-01-05T00:00:00.000000000+00:00"]
```

Inclusiveness never inserts an off-grid endpoint.

## Example 3: Descending Calendar Days

```cpp
const auto out = pluss::timeline::date_range(
    pluss::timeline::date("2024-01-05"),
    pluss::timeline::date("2024-01-01"), "D", -2);
std::cout << pluss::note::print(
    pluss::timeline::format(out, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-01-05T00:00:00.000000000+00:00", "2024-01-03T00:00:00.000000000+00:00", "2024-01-01T00:00:00.000000000+00:00"]
```

## Example 4: A Direction Mismatch Is Empty

```cpp
const auto empty = pluss::timeline::date_range(
    pluss::timeline::date("2024-01-05"),
    pluss::timeline::date("2024-01-01"), "D", 1);
std::cout << pluss::timeline::print(empty) << '\n';
```

### Output

```text
Timeline(shape=[0], tz=UTC)[  ]
```

## Example 5: Calendar Days Preserve Local Clock Time

```cpp
const auto start = pluss::timeline::make("2024-03-09T14:30:00Z")
                       .tz_convert("America/New_York");
const auto end = pluss::timeline::make("2024-03-11T13:30:00Z")
                     .tz_convert("America/New_York");
const auto days = pluss::timeline::date_range(start, end, "D");

std::cout << pluss::note::print(
    pluss::timeline::format(days, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-03-09T09:30:00.000000000-05:00", "2024-03-10T09:30:00.000000000-04:00", "2024-03-11T09:30:00.000000000-04:00"]
```

The first interval is 82,800,000,000,000 nanoseconds, or 23 elapsed hours; the
second is 86,400,000,000,000 nanoseconds. Both are one local calendar day.

## Example 6: Hours Advance Through The Transition

```cpp
const auto start = pluss::timeline::make("2024-03-10T05:00:00Z")
                       .tz_convert("America/New_York");
const auto end = pluss::timeline::make("2024-03-10T08:00:00Z")
                     .tz_convert("America/New_York");
const auto hours = pluss::timeline::date_range(start, end, "H");

std::cout << pluss::note::print(
    pluss::timeline::format(hours, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-03-10T00:00:00.000000000-05:00", "2024-03-10T01:00:00.000000000-05:00", "2024-03-10T03:00:00.000000000-04:00", "2024-03-10T04:00:00.000000000-04:00"]
```

Local `02:00` does not exist, but adjacent ticks remain one elapsed hour apart.

## Validation And Failure

Both endpoints must be scalar, defined instants. `step` must be nonzero and
must not overflow its unit. Unsupported frequencies throw. A calendar-day step
that reconstructs a nonexistent local clock also throws rather than silently
changing the requested clock.

```cpp
try {
  const auto a = pluss::timeline::date("2024-01-01");
  const auto b = pluss::timeline::date("2024-01-02");
  (void)pluss::timeline::date_range(a, b, "D", 0);
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`arange`](arange.md): use a `Duration` step or exclude the endpoint.
- [`date`](date.md): construct scalar date endpoints.
- [`from_epoch`](from_epoch.md): inspect or rebuild exact ticks.
