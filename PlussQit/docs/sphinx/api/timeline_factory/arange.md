<!-- pluss-hand-authored -->
# pluss::timeline::arange

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::arange(const Timeline& start,
                                 const Timeline& end,
                                 const Duration& step,
                                 bool inclusive = true)
Timeline pluss::timeline::arange(std::string_view start,
                                 std::string_view end,
                                 const Duration& step,
                                 bool inclusive = true)
Timeline pluss::timeline::arange(std::string_view start,
                                 std::string_view end,
                                 std::int64_t step = 1,
                                 bool inclusive = true)
```

## Build A Sequence From An Exact Step

`arange()` begins at `start`, repeatedly adds one scalar `Duration`, and returns
the generated instants as a rank-one `Timeline`. It is the appropriate range
factory when the interval is already known as elapsed nanoseconds, seconds,
hours, or another exact duration. Use `date_range()` instead when a named
calendar frequency, especially a local calendar day, is the intended rule.

The `Timeline` overload accepts any two scalar instants. The text overload with
a `Duration` accepts matching date-only, time-only, or complete timestamp text.
The integer overload is deliberately narrower: an integer means elapsed days
for date-only text and elapsed seconds for time-only text. Complete timestamps
must use an explicit `Duration`, preventing a bare integer from acquiring a
surprising unit.

`inclusive=true` includes `end` only when repeated stepping lands on it.
`inclusive=false` always excludes `end`. Neither setting inserts an off-grid
endpoint. A positive step advances toward a later endpoint; a negative step
advances toward an earlier endpoint. A sign pointing away from the endpoint
produces an empty vector rather than reversing the sign.

Every step is elapsed time. A 24-hour `Duration` is always 86,400 seconds; it is
not a promise to preserve a displayed local clock through an offset change. The
result retains `start.tz()`, while `end` is used only as an instant boundary.

## Example 1: Timeline Endpoints And An Open Boundary

```cpp
const auto ticks = pluss::timeline::arange(
    pluss::timeline::from_epoch(0LL),
    pluss::timeline::from_epoch(7LL),
    pluss::duration::nanos(2),
    false);
std::cout << pluss::note::print(
    pluss::timeline::format(ticks, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["1970-01-01T00:00:00.000000000+00:00", "1970-01-01T00:00:00.000000002+00:00", "1970-01-01T00:00:00.000000004+00:00", "1970-01-01T00:00:00.000000006+00:00"]
```

The exact two-nanosecond interval is retained. Seven nanoseconds is off-grid,
so changing `inclusive` to `true` would not append it.

## Example 2: Date Text Gives Integer Steps A Day Unit

```cpp
const auto days = pluss::timeline::arange(
    "2024-02-27", "2024-03-02", 2);
std::cout << pluss::note::print(
    pluss::timeline::format(days, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-02-27T00:00:00.000000000+00:00", "2024-02-29T00:00:00.000000000+00:00", "2024-03-02T00:00:00.000000000+00:00"]
```

The sequence crosses leap day without special casing because each integer step
means an exact 24-hour duration from a UTC date anchor.

## Example 3: Time Text Gives Integer Steps A Second Unit

```cpp
const auto seconds = pluss::timeline::arange(
    "09:30:00", "09:30:03", 1, false);
std::cout << pluss::note::print(
    pluss::timeline::format(seconds, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["1970-01-01T09:30:00.000000000+00:00", "1970-01-01T09:30:01.000000000+00:00", "1970-01-01T09:30:02.000000000+00:00"]
```

Clock-only text is anchored to 1970-01-01 UTC, just as it is for `time()`.

## Example 4: Descending Sequences

```cpp
const auto descending = pluss::timeline::arange(
    "2024-03-03", "2024-02-28", -2);
std::cout << pluss::note::print(
    pluss::timeline::format(descending, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-03-03T00:00:00.000000000+00:00", "2024-03-01T00:00:00.000000000+00:00", "2024-02-28T00:00:00.000000000+00:00"]
```

## Example 5: A Direction Mismatch Is Empty

```cpp
const auto empty = pluss::timeline::arange(
    "2024-03-03", "2024-02-28", 2);
std::cout << pluss::timeline::print(empty) << '\n';
```

### Output

```text
Timeline(shape=[0], tz=UTC)[  ]
```

The result is a defined rank-one value with zero elements.

## Example 6: Exact Hours Across An Offset Transition

```cpp
const auto first = pluss::timeline::make("2024-03-10T05:00:00Z")
                       .tz_convert("America/New_York");
const auto last = pluss::timeline::make("2024-03-10T08:00:00Z")
                      .tz_convert("America/New_York");
const auto hours = pluss::timeline::arange(
    first, last, pluss::duration::hours(1));

std::cout << pluss::note::print(
    pluss::timeline::format(hours, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-03-10T00:00:00.000000000-05:00", "2024-03-10T01:00:00.000000000-05:00", "2024-03-10T03:00:00.000000000-04:00", "2024-03-10T04:00:00.000000000-04:00"]
```

Adjacent instants remain one elapsed hour apart. The local clock skips 02:00
because that wall time does not exist during this transition.

## Validation And Failure

All three object arguments must be scalar. Endpoints and the step must be
defined, and the step must be nonzero. Text endpoints must have the same kind;
date text cannot be paired with clock text. Complete timestamp text rejects an
integer step. Generation stops safely before integer addition would overflow.

```cpp
try {
  const auto start = pluss::timeline::date("2024-01-01");
  const auto end = pluss::timeline::date("2024-01-03");
  (void)pluss::timeline::arange(
      start, end, pluss::duration::nanos(pluss::Duration::kNaT));
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`date_range`](date_range.md): advance by a named elapsed or calendar unit.
- [`date`](date.md): construct strict UTC date endpoints.
- [`time`](time.md): construct epoch-anchored clock endpoints.

