# Timeline::period_start

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::period_start(std::string_view unit) const
```

## Find The Beginning Of Each Containing Time Unit

`period_start()` replaces every defined instant with the first instant of the
local calendar unit that contains it. It answers questions such as "when did
this value's month begin?" and "which local hour contains this event?" while
remaining a `Timeline` operation: the result still contains exact instants,
not labels, interval objects, or integer group identifiers.

This is useful when an instant is needed as a canonical boundary. Values that
belong to the same month, for example, receive the same month-start instant
and can then be compared or used as lower bounds. If the intended result is a
calendar component such as the number `7`, use `month()` instead. If the
intended result is a distance between two values, use `span()` or `diff()`.

The accepted units are deliberately narrow and canonical:

| `unit` | Retained local fields | Fields reset to their beginning |
| --- | --- | --- |
| `"year"` | year | month and day become January 1; clock becomes midnight |
| `"month"` | year and month | day becomes 1; clock becomes midnight |
| `"day"` | year, month, and day | clock becomes midnight |
| `"hour"` | date and hour | minute, second, and fraction become zero |
| `"minute"` | date, hour, and minute | second and fraction become zero |
| `"second"` | date, hour, minute, and second | fractional nanoseconds become zero |

Matching is ASCII case-insensitive, so `"YEAR"` and `"year"` name the same
unit. Abbreviations and plural spellings are not aliases: `"Y"`, `"min"`,
and `"years"` are rejected. One vocabulary avoids ambiguous call sites and
keeps configuration text consistent with the named Timeline methods.

## Example 1: See Every Boundary From One Instant

The source below is noon in Edmonton during daylight-saving time. Each call
works from the displayed local calendar, not by truncating the stored UTC
nanoseconds.

```cpp
const auto value = pluss::timeline::make(
    "2024-07-15T18:34:56.123456789Z")
    .tz_convert("America/Edmonton");

auto show = [](std::string_view label, const pluss::Timeline& result) {
  std::cout << "\n=== " << label << " ===\n"
            << pluss::note::print(
                                  pluss::timeline::format(result, "%FT%T.%f%z"))
            << '\n';
};

show("source", value);
show("year", value.period_start("year"));
show("month", value.period_start("month"));
show("day", value.period_start("day"));
show("hour", value.period_start("hour"));
show("minute", value.period_start("minute"));
show("second", value.period_start("second"));
```

### Output

```text
=== source ===
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-07-15T12:34:56.123456789-06:00"

=== year ===
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-01-01T00:00:00.000000000-07:00"

=== month ===
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-07-01T00:00:00.000000000-06:00"

=== day ===
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-07-15T00:00:00.000000000-06:00"

=== hour ===
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-07-15T12:00:00.000000000-06:00"

=== minute ===
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-07-15T12:34:00.000000000-06:00"

=== second ===
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-07-15T12:34:56.000000000-06:00"
```

The year boundary uses `-07:00`, while the July boundaries use `-06:00`.
`period_start()` preserves the timezone and the requested civil boundary; it
does not preserve the source's numeric UTC offset when the boundary belongs to
a different offset season.

## Local Boundaries, Not Elapsed Buckets

For timezone-aware values, every unit is resolved in `tz()`. A day starts at
local midnight even when the elapsed distance from the preceding midnight is
23 or 25 hours. Year and month starts are reconstructed from local calendar
fields. Day through second starts use local wall-clock flooring.

This makes `period_start("day")` a calendar operation. It is not equivalent to
dividing epoch nanoseconds by 86,400 seconds, which would create UTC-aligned
buckets and assign the wrong local date in many timezones.

Applying the same unit twice is idempotent in value:

```cpp
const auto once = value.period_start("month");
const auto twice = once.period_start("month");
std::cout << "\n=== idempotent ===\n"
          << std::boolalpha << once.eq(twice).all().item<bool>() << '\n';
```

### Output

```text
=== idempotent ===
true
```

The method is nevertheless out-of-place. It allocates a result and leaves the
receiver unchanged, even when every input already lies on the requested
boundary.

## Example 2: Shape, `NaT`, Leap Dates, And Pre-Epoch Values

Projection is elementwise and shape-preserving. `NaT` remains `NaT`; it is not
interpreted as an old date and does not make neighboring lanes fail.

```cpp
const auto matrix = pluss::timeline::make({
    {"2024-02-29T23:59:59.999999999Z", ""},
    {"1969-12-31T23:59:59.500000000Z", "2025-01-01T00:00:00Z"},
});

std::cout << pluss::note::print(pluss::timeline::format(
    matrix.period_start("month"), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-02-01T00:00:00.000000000+00:00", "NaT"], 
  ["1969-12-01T00:00:00.000000000+00:00", "2025-01-01T00:00:00.000000000+00:00"]
]
```

Leap-day input does not require a special policy because the result is the
first day of February. Negative epoch values use mathematical calendar
boundaries, so the December 1969 lane stays in December rather than being
truncated toward the Unix epoch.

## Example 3: Repeated Local Hours Retain Their Identity

When clocks move backward, one wall-clock hour occurs twice. Flooring either
occurrence must retain which occurrence contained the source instant.

```cpp
const auto folds = pluss::timeline::make({
    "2024-11-03T07:45:00Z",
    "2024-11-03T08:45:00Z",
}).tz_convert("America/Edmonton");

std::cout << pluss::note::print(pluss::timeline::format(
    folds.period_start("hour"), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-11-03T01:00:00.000000000-06:00", "2024-11-03T01:00:00.000000000-07:00"]
```

Both results display `01:00`, but they remain different instants because their
offsets differ. This distinction is preserved in `ticks()`, comparisons, and
later elapsed-time arithmetic.

## Example 4: Non-Contiguous Input Uses Logical Order

`period_start()` accepts the same strided Timeline storage as the rest of the
public surface. It reads logical lanes through their strides and returns a
contiguous owning result.

```cpp
auto storage = pluss::timeline::make({
    "2024-01-15T12:00:00Z", "2030-01-01T00:00:00Z",
    "2024-03-20T18:00:00Z", "2030-01-01T00:00:00Z",
}).ticks();
const std::array<std::int64_t, 1> sizes{2};
const std::array<std::int64_t, 1> strides{2};
const auto strided = pluss::Timeline::from_buffer(
    storage.data(), sizes, strides, {}, "UTC");

std::cout << pluss::note::print(pluss::timeline::format(
    strided.period_start("year"), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-01-01T00:00:00.000000000+00:00", "2024-01-01T00:00:00.000000000+00:00"]
```

The two interleaved 2030 storage positions are not part of the logical view.

## Example 5: Empty Shapes Remain Empty

```cpp
const auto empty = pluss::timeline::from_epoch(
    std::vector<std::int64_t>{}, {2, 0});
std::cout << pluss::note::print(pluss::timeline::format(
    empty.period_start("day"), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 0], dim=2, numel=0, encoding=utf8)
[]
```

The rank and zero extent are retained. Unit validation still occurs before
element traversal, so an unsupported unit throws even for an empty Timeline.

## Example 6: Invalid And Removed Spellings Fail Clearly

```cpp
const auto value = pluss::timeline::make("2024-07-15T18:34:56Z");

try {
  (void)value.period_start("years");
} catch (const pluss::Error&) {
  std::cout << "\n=== plural_alias ===\nrejected\n";
}
try {
  (void)value.period_start("week");
} catch (const pluss::Error&) {
  std::cout << "\n=== unsupported_unit ===\nrejected\n";
}
```

### Output

```text
=== plural_alias ===
rejected

=== unsupported_unit ===
rejected
```

`"years"` demonstrates that plural compatibility aliases are intentionally
absent. `"week"` is not supported because a week boundary requires an explicit
weekday convention; this method does not silently choose one.

Timezone lookup and local-boundary resolution can also throw when timezone
metadata is invalid or when a requested boundary cannot be represented as an
epoch-nanosecond instant. No partial result is returned.

## Choosing A Neighboring Operation

- Use [`floor`](floor.md) for second-through-day flooring when its compact unit
  vocabulary is already appropriate at the call site.
- Use [`date`](date.md) when the request is specifically local midnight for
  each value's date.
- Use [`year`](year.md), [`month`](month.md), or another field projection when
  the result should be an integer component rather than a boundary instant.
- Use [`shift`](shift.md) to move values by units instead of locating the
  beginning of their current unit.
- Use [`span`](span.md) or [`diff`](diff.md) to measure between two Timelines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operators.html"><span class="label">Previous</span><span class="title">operators</span></a>
  <a class="next-link" href="quarter.html"><span class="label">Next</span><span class="title">quarter</span></a>
</div>
<!-- pluss-hand-authored -->
