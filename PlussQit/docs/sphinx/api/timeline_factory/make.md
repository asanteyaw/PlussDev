<!-- pluss-hand-authored -->
# pluss::timeline::make

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::make(std::string_view value)
Timeline pluss::timeline::make(string_range values)
Timeline pluss::timeline::make(nested_string_values values)
Timeline pluss::timeline::make(string_range flat, integer_range sizes)
```

## Parse Timestamp Text Into Instants

`make()` is the general text entrance to `Timeline`. Use it when each token may
contain a complete timestamp, a date alone, or an explicit numeric UTC offset.
It parses every token into one signed epoch-nanosecond tick and returns a UTC
timeline with the shape implied by the selected call form.

Parsing an instant and remembering its source spelling are different jobs.
`make()` retains the instant, not the original separator, offset spelling, or
fractional width. Consequently, `2024-01-01T00:00:00Z` and
`2023-12-31T17:00:00-07:00` become equal ticks. Use `format()` later to choose
one consistent representation.

Choose a narrower factory when the grammar itself is part of the contract:
`date()` rejects clock fields, while `time()` rejects calendar fields. An empty
or whitespace-only token is the deliberate text spelling for `NaT`; malformed
nonempty text throws.

## Accepted Text

The documented grammar includes:

- dates as `YYYY-MM-DD` or `YYYY/MM/DD`
- a date followed by a clock, separated by `T` or a space
- `HH:MM`, `HH:MM:SS`, and fractional seconds
- `Z`, `+HH:MM`, `-HH:MM`, `+HHMM`, or `-HHMM` suffixes
- surrounding ASCII whitespace

A token without an offset is interpreted as UTC. Fractions shorter than nine
digits are right-padded to nanoseconds. Digits beyond nanosecond precision are
discarded rather than rounded. Calendar dates, clock fields, offsets, and the
representable nanosecond range are validated before a result is returned.

## Example 1: Scalar Input Produces Rank Zero

```cpp
const auto event =
    pluss::timeline::make("2024-02-29T23:30:45.123456789-07:00");
std::cout << pluss::note::print(
    pluss::timeline::format(event, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-01T06:30:45.123456789+00:00"
```

The date changes because the source text describes an instant seven hours
behind UTC. The result is a scalar, not a one-element vector.

## Example 2: One-Dimensional Mixed Forms And `NaT`

```cpp
const auto events = pluss::timeline::make({
    "2024-02-29", "2024/03/01 06:15",
    "2024-03-01T08:15:00+0200", ""});
std::cout << pluss::note::print(
    pluss::timeline::format(events, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-02-29T00:00:00.000000000+00:00", "2024-03-01T06:15:00.000000000+00:00", "2024-03-01T06:15:00.000000000+00:00", "NaT"]
```

Date-only input starts at midnight. The middle two spellings resolve to the
same instant. The empty token creates absence without weakening other lanes.

## Example 3: Nested Braces Infer Matrix Shape

```cpp
const auto matrix = pluss::timeline::make({
    {"2024-01-01", "2024-01-02T03:04:05Z"},
    {"2024-01-03T06:07Z", "2024-01-04T08:09:10.5Z"},
});
std::cout << pluss::note::print(
    pluss::timeline::format(matrix, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-01T00:00:00.000000000+00:00", "2024-01-02T03:04:05.000000000+00:00"], 
  ["2024-01-03T06:07:00.000000000+00:00", "2024-01-04T08:09:10.500000000+00:00"]
]
```

Nested rows must be rectangular. Shape is inferred before parsing, and
Timeline accepts ranks zero through three.

## Example 4: Runtime Flat Storage With Explicit Geometry

```cpp
const std::vector<std::string> flat{
    "2024-04-01", "2024-04-02", "2024-04-03", "2024-04-04"};
const auto cube = pluss::timeline::make(flat, {1, 2, 2});
std::cout << pluss::note::print(
    pluss::timeline::format(cube, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[1, 2, 2], dim=3, numel=4, encoding=utf8)
[
  [
    ["2024-04-01T00:00:00.000000000+00:00", "2024-04-02T00:00:00.000000000+00:00"], 
    ["2024-04-03T00:00:00.000000000+00:00", "2024-04-04T00:00:00.000000000+00:00"]
  ]
]
```

The flat range is interpreted in row-major order. Its cardinality must equal
the product of `sizes`; zero extents are allowed, negative extents and ranks
above three are not.

## Example 5: Offsets Identify Instants

```cpp
const auto same = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2023-12-31T17:00:00-07:00"});
pluss::series::print(std::cout, same.epoch_nanos());
```

### Output

```text
Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1704067200000000000, 1704067200000000000]
```

## Failure Modes

The entire construction fails if any defined token is invalid. Representative
failures include impossible dates, invalid numeric offsets, ragged nesting,
shape/cardinality mismatch, and timestamps outside the signed nanosecond range.

```cpp
try {
  (void)pluss::timeline::make("2023-02-29");
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`date`](date.md): require date-only tokens.
- [`time`](time.md): require clock-only tokens.
- [`from_epoch`](from_epoch.md): construct from exact integer ticks.
- [`format`](format.md): render parsed instants with controlled text.
