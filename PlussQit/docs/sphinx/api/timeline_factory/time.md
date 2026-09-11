<!-- pluss-hand-authored -->
# pluss::timeline::time

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::time(std::string_view value)
Timeline pluss::timeline::time(string_range values)
Timeline pluss::timeline::time(nested_string_values values)
Timeline pluss::timeline::time(string_range flat, integer_range sizes)
Timeline pluss::timeline::time(const Timeline& values)
```

## Turn Clock Readings Into Comparable Instants

`time()` constructs anchored clock values in two ways. Text overloads parse
clock-only input. The Timeline overload extracts each instant's local hour,
minute, second, and nanosecond fields. Both place those fields on `1970-01-01`
because a `Timeline` always stores complete instants and therefore cannot store
a clock with no date at all.

Use `time()` for time-of-day values that need ordering, duration arithmetic,
shape, or exact nanosecond storage. Use the Timeline overload when the source
already contains complete timestamps; use `make()` when the source date must
remain part of the result.

The default anchor timezone is UTC. A numeric offset on the clock is accepted
and identifies an instant relative to that UTC anchor. For example,
`09:30:00-07:00` becomes `16:30:00Z`. This may be useful for protocol fields,
but a named civil timezone and its daylight-saving rules require an actual
date; an isolated clock cannot determine which seasonal offset applies.

Accepted clocks are `HH:MM` or `HH:MM:SS`, optionally followed by fractional
seconds and a numeric offset. Hours are `0..23`, minutes and seconds are
`0..59`, and leap-second spelling `60` is rejected. Fractions are retained to
nanosecond precision.

For a Timeline input, extraction occurs in `values.tz()`, then the clock is
re-localized on the anchor date in that same timezone. This matters when the
source date and 1970 anchor use different UTC offsets. Rank, shape, timezone,
ordering, and lane-wise `NaT` are preserved. No lane without a time part exists
inside a defined Timeline; `NaT` is the absent representation and propagates.

## Example 1: One Nanosecond After The Anchor

```cpp
const auto tick = pluss::timeline::time("00:00:00.000000001");
std::cout << pluss::note::print(
    pluss::timeline::format(tick, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"1970-01-01T00:00:00.000000001+00:00"
```

The scalar has shape `[]`. The explicit formatter is used because the compact
Timeline printer does not expose fractional nanoseconds.

## Example 2: Minute And Fractional Forms

```cpp
const auto clocks = pluss::timeline::time({"09:30", "17:45:30.250"});
std::cout << pluss::note::print(
    pluss::timeline::format(clocks, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["1970-01-01T09:30:00.000000000+00:00", "1970-01-01T17:45:30.250000000+00:00"]
```

Omitted seconds become zero. A three-digit fraction means 250 milliseconds,
which occupies the first three of nine fractional digits.

## Example 3: Numeric Offset Normalization

```cpp
const auto clock = pluss::timeline::time("09:30:00-07:00");
std::cout << pluss::note::print(
    pluss::timeline::format(clock, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"1970-01-01T16:30:00.000000000+00:00"
```

The source offset is consumed to determine the instant; it is not retained as
display metadata.

## Example 4: Explicit Matrix Shape

```cpp
const std::vector<std::string> flat{
    "00:00:00", "06:00:00", "12:00:00", "18:00:00"};
const auto matrix = pluss::timeline::time(flat, {2, 2});
std::cout << pluss::note::print(
    pluss::timeline::format(matrix, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["1970-01-01T00:00:00.000000000+00:00", "1970-01-01T06:00:00.000000000+00:00"], 
  ["1970-01-01T12:00:00.000000000+00:00", "1970-01-01T18:00:00.000000000+00:00"]
]
```

Nested braces can infer the same shape. Flat-plus-shape is preferable when the
values and geometry arrive separately at runtime.

## Example 5: Extract Clock Values From A Timeline

```cpp
const auto events = pluss::timeline::make({
    "2024-07-15T18:34:56.123456789Z", "",
}).tz_convert("America/Edmonton");
std::cout << pluss::timeline::print(pluss::timeline::time(events)) << '\n';
```

### Output

```text
Timeline(shape=[2], tz=America/Edmonton)
[ 1970-01-01 12:34:56, NaT ]
```

The compact printer hides the fraction, but the stored first lane retains
`.123456789`. The overload is equivalent to `events.time()` and leaves `events`
unchanged.

## Example 6: Nested Clock Text Constructs Directly

```cpp
const auto clocks = pluss::timeline::time({
    {"00:00:00", "06:00:00"},
    {"12:00:00", "18:00:00"},
});
std::cout << pluss::timeline::print(clocks) << '\n';
```

### Output

```text
Timeline(shape=[2, 2], tz=UTC)
[ 1970-01-01 00:00:00 1970-01-01 06:00:00 ]
[ 1970-01-01 12:00:00 1970-01-01 18:00:00 ]
```

This is the direct nested-brace construction form. No intermediate Timeline is
required.

## Validation And Failure

For text overloads, dates, empty tokens, impossible clock fields, invalid
offsets, ragged nesting, shape mismatches, and ranks above three throw. No
wrapping is performed:
`24:00:00` does not become the following day, and `23:59:60` does not become
midnight.

```cpp
try {
  (void)pluss::timeline::time("23:59:60");
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`date`](date.md): require calendar dates and anchor at midnight.
- [`make`](make.md): parse timestamps containing both date and time.
- [`full`](full.md): fill a requested shape with one parsed temporal value.
