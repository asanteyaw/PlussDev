<!-- pluss-hand-authored -->
# pluss::timeline::format

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Note pluss::timeline::format(const Timeline& value,
                             std::string_view pattern)
Note pluss::timeline::format(
    const Timeline& value,
    const pluss::timeline::FormatOptions& options = {})
```

```cpp
struct pluss::timeline::FormatOptions {
  std::string pattern{"%Y-%m-%dT%H:%M:%S.%f%z"};
  std::string timezone{};
  std::string nat{"NaT"};
  int fractional_digits{9};
  bool trim_fractional_zeros{false};
};
```

## Convert Instants Into Shape-Preserving Text

`format()` renders every instant into a `Note` while preserving the input rank,
shape, and row-major element order. It exists for structured text that remains
usable by later string operations. Use `print()` instead for a single diagnostic
string containing headers and layout.

Formatting never changes the source `Timeline`, its epoch nanoseconds, or its
timezone metadata. Calendar and clock directives are evaluated in the source
timezone unless `FormatOptions::timezone` names a projection timezone. That
projection is applied independently to every instant, so seasonal offsets are
not cached once for the whole value.

The pattern-only overload changes only `pattern`; all other options retain their
defaults. The options overload controls the complete rendering contract:

- `pattern` supplies literal text and `%` directives.
- `timezone` selects a named display zone; an empty string uses `value.tz()`.
- `nat` is the complete output token for each absent instant. The pattern is not
  applied to that lane.
- `fractional_digits` selects 1 through 9 leading nanosecond digits for `%f` and
  `%N`.
- `trim_fractional_zeros` removes trailing zeros from those digits but retains
  at least one digit.

Reducing fractional precision truncates; it does not round the instant. Pattern
compilation and validation occur once per call, then the compiled pieces are
reused for every lane. The result owns its strings.

## Directive Reference

- `%Y`, `%m`, `%d`: year, two-digit month, and two-digit day.
- `%H`, `%M`, `%S`: two-digit hour, minute, and second.
- `%F`: `%Y-%m-%d`; `%T`: `%H:%M:%S`; `%R`: `%H:%M`.
- `%f`, `%N`: fractional nanoseconds under the configured precision policy.
- `%a`, `%A`: abbreviated and full English weekday name.
- `%b`, `%B`: abbreviated and full English month name.
- `%j`: day of year from `001` through `366`.
- `%u`: weekday `1` through `7`, Monday first.
- `%w`: weekday `0` through `6`, Sunday first.
- `%q`: calendar quarter `1` through `4`.
- `%z`: numeric UTC offset such as `-07:00`.
- `%Z`: the selected timezone label.
- `%%`: a literal percent sign.

## Example 1: Defaults Preserve Matrix Shape And Nanoseconds

```cpp
const auto values = pluss::timeline::make({
    {"2024-02-29T23:30:45.123456789Z", "2024-03-01T00:00:00Z"},
    {"2024-07-01T06:00:00Z", ""},
});
std::cout << pluss::note::print(pluss::timeline::format(values)) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-02-29T23:30:45.123456789+00:00", "2024-03-01T00:00:00.000000000+00:00"], 
  ["2024-07-01T06:00:00.000000000+00:00", "NaT"]
]
```

## Example 2: The Pattern Overload

```cpp
const auto values = pluss::timeline::make({
    {"2024-02-29T23:30:45.123456789Z", "2024-03-01T00:00:00Z"},
    {"2024-07-01T06:00:00Z", ""},
});
const auto labels = pluss::timeline::format(
    values, "%a %b %d, day=%j weekday=%u/%w quarter=Q%q");
std::cout << pluss::note::print(labels) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["Thu Feb 29, day=060 weekday=4/4 quarter=Q1", "Fri Mar 01, day=061 weekday=5/5 quarter=Q1"], 
  ["Mon Jul 01, day=183 weekday=1/1 quarter=Q3", "NaT"]
]
```

The absent lane bypasses all directives and remains the default `NaT` token.

## Example 3: Timezone Projection Is Per Instant

```cpp
auto options = pluss::timeline::FormatOptions{};
options.pattern = "%F %T %Z %z";
options.timezone = "America/New_York";

const auto seasons = pluss::timeline::make({
    "2024-01-15T12:00:00Z", "2024-07-15T12:00:00Z"});
std::cout << pluss::note::print(
    pluss::timeline::format(seasons, options)) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-01-15 07:00:00 America/New_York -05:00", "2024-07-15 08:00:00 America/New_York -04:00"]
```

The two rows use different offsets because the zone rules are resolved at each
instant.

## Example 4: Fractional Precision And Trimming

```cpp
auto options = pluss::timeline::FormatOptions{};
options.pattern = "%T.%f | %N";
options.fractional_digits = 6;
options.trim_fractional_zeros = true;

const auto fractions = pluss::timeline::make({
    "2024-01-01T00:00:00.120000000Z",
    "2024-01-01T00:00:00.123456789Z"});
std::cout << pluss::note::print(
    pluss::timeline::format(fractions, options)) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["00:00:00.12 | 12", "00:00:00.123456 | 123456"]
```

The last three digits of the second instant are omitted, not rounded into the
sixth digit.

## Example 5: Literal Percent And A Custom Absent Token

```cpp
auto options = pluss::timeline::FormatOptions{};
options.pattern = "observed=%F %% %T";
options.nat = "not-observed";

const auto observations = pluss::timeline::make({
    "", "2024-01-02T03:04:05Z"});
std::cout << pluss::note::print(
    pluss::timeline::format(observations, options)) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["not-observed", "observed=2024-01-02 % 03:04:05"]
```

## Example 6: Empty Geometry Survives Formatting

```cpp
const auto empty = pluss::timeline::from_epoch(
    std::vector<std::int64_t>{}, {2, 0});
std::cout << pluss::note::print(
    pluss::timeline::format(empty, "%F")) << '\n';
```

### Output

```text
Note(shape=[2, 0], dim=2, numel=0, encoding=utf8)
[]
```

## Validation And Failure

An unsupported directive, a trailing `%`, an unknown timezone, or a
`fractional_digits` value outside `[1, 9]` rejects the complete call before a
partial `Note` is returned. Timezone and pattern validation still occur for an
empty input.

```cpp
try {
  const auto values = pluss::timeline::make("2024-01-01T00:00:00Z");
  (void)pluss::timeline::format(values, "%Y-%Q");
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`print`](print.md): produce one diagnostic rendering string.
- [`make`](make.md): parse textual instants.
- [`from_epoch`](from_epoch.md): preserve and inspect exact integer ticks.
