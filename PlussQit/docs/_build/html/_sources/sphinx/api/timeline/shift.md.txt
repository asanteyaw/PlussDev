# Timeline::shift

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::shift(int periods, std::string_view unit) const
```

## Move Instants By A Runtime-Selected Unit

`shift()` moves every defined instant by the same signed number of temporal
units and returns a new `Timeline`. It exists for code in which the unit is a
runtime choice: a parser, command line, configuration file, or user interface
can supply `unit` without requiring a branch at the call site. When the unit is
known while writing C++, the named operation such as [`add_days()`](add_days.md)
or [`add_hours()`](add_hours.md) states the intent more directly.

The method preserves the receiver's shape, logical element order, and timezone.
`NaT` remains `NaT`. It neither mutates the receiver nor shifts positions along
an axis; "shift" here means temporal arithmetic on each value.

## Parameters And Units

| Parameter | Meaning | Accepted values |
| --- | --- | --- |
| `periods` | Signed count applied to every defined element | Negative, zero, or positive `int` |
| `unit` | Temporal unit selected at runtime | `year`, `month`, `day`, `hour`, `minute`, `second`, compared without regard to ASCII letter case |

There are deliberately no abbreviated or plural aliases. For example, `DAY`
is accepted, but `d`, `days`, and ` day ` are not. The method does not trim
whitespace. Keeping one spelling per concept prevents configuration text from
quietly acquiring a second vocabulary.

The units divide into two arithmetic models:

| Unit | Arithmetic | Equivalent named method |
| --- | --- | --- |
| `year` | Local-calendar movement with end-of-month clamping | `add_years(periods)` |
| `month` | Local-calendar movement with end-of-month clamping | `add_months(periods)` |
| `day` | Local-calendar movement preserving the displayed clock | `add_days(periods)` |
| `hour` | Fixed elapsed duration | `add_hours(periods)` |
| `minute` | Fixed elapsed duration | `add_minutes(periods)` |
| `second` | Fixed elapsed duration | `add_seconds(periods)` |

Calendar movement is evaluated in `tz()`. Fixed-duration movement is evaluated
on the underlying instant. That difference becomes visible at daylight-saving
boundaries.

## Example: Dispatch Every Supported Unit

```cpp
const auto leap = pluss::timeline::make("2024-02-29T08:15:30Z");

std::cout << pluss::timeline::print(leap.shift(1, "year")) << '\n';
std::cout << pluss::timeline::print(leap.shift(1, "month")) << '\n';
std::cout << pluss::timeline::print(leap.shift(1, "day")) << '\n';
std::cout << pluss::timeline::print(leap.shift(1, "hour")) << '\n';
std::cout << pluss::timeline::print(leap.shift(1, "minute")) << '\n';
std::cout << pluss::timeline::print(leap.shift(1, "second")) << '\n';
```

### Output

```text
Timeline(shape=[], tz=UTC)
2025-02-28 08:15:30
Timeline(shape=[], tz=UTC)
2024-03-29 08:15:30
Timeline(shape=[], tz=UTC)
2024-03-01 08:15:30
Timeline(shape=[], tz=UTC)
2024-02-29 09:15:30
Timeline(shape=[], tz=UTC)
2024-02-29 08:16:30
Timeline(shape=[], tz=UTC)
2024-02-29 08:15:31
```

The year result demonstrates end-of-month clamping: one calendar year after
29 February 2024 is 28 February 2025. Month movement keeps day 29 because that
date exists in March. Day movement crosses the leap-day boundary normally.

## Example: Negative, Zero, And Case-Insensitive Dispatch

```cpp
const auto leap = pluss::timeline::make("2024-02-29T08:15:30Z");

std::cout << pluss::timeline::print(leap.shift(-1, "DAY")) << '\n';
std::cout << pluss::timeline::print(leap.shift(0, "month")) << '\n';
```

### Output

```text
Timeline(shape=[], tz=UTC)
2024-02-28 08:15:30
Timeline(shape=[], tz=UTC)
2024-02-29 08:15:30
```

A negative count moves backward. Zero still returns a value-equivalent new
timeline through the selected operation; it is useful when a signed runtime
count naturally includes zero, but should not be used merely to copy an object.

## Example: Civil Days Are Not Always Twenty-Four Hours

```cpp
const auto before = pluss::timeline::make("2024-03-09T10:30:00Z")
                        .tz_convert("America/Edmonton");

std::cout << pluss::timeline::print(before.shift(1, "day")) << '\n';
std::cout << pluss::timeline::print(before.shift(24, "hour")) << '\n';
```

### Output

```text
Timeline(shape=[], tz=America/Edmonton)
2024-03-10 03:30:00
Timeline(shape=[], tz=America/Edmonton)
2024-03-10 04:30:00
```

The first operation preserves the local clock while crossing the spring
transition, so only 23 elapsed hours pass. The second adds exactly 24 elapsed
hours, so its displayed clock is one hour later. Choose `day` for instructions
about the next local calendar day and `hour` for elapsed-time requirements.

## Example: Shape, Timezone, And `NaT` Are Preserved

```cpp
const auto matrix = pluss::timeline::make({
    {"2024-01-01T00:00:00Z", ""},
    {"2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z"},
}).tz_convert("America/Edmonton");

std::cout << pluss::timeline::print(matrix.shift(2, "hour")) << '\n';
```

### Output

```text
Timeline(shape=[2, 2], tz=America/Edmonton)
[ 2023-12-31 19:00:00                 NaT ]
[ 2024-01-02 19:00:00 2024-01-03 19:00:00 ]
```

The scalar count broadcasts over all logical positions without changing the
two-dimensional geometry. A `NaT` position is propagated rather than treated
as an invalid calendar value.

## Example: Noncanonical Text Is Rejected

```cpp
const auto t = pluss::timeline::make("2024-02-29T08:15:30Z");

for (const auto unit : {"years", " day "}) {
  try {
    (void)t.shift(1, unit);
  } catch (const pluss::Error&) {
    std::cout << "rejected\n";
  }
}
```

### Output

```text
rejected
rejected
```

An unsupported unit fails before a result is returned. For a supported unit,
calendar-range or duration overflow also throws. Calendar movement is
all-or-nothing: the method does not return a partially shifted result if one
defined element cannot be represented.

## Cost And Allocation

`shift()` is linear in [`numel()`](numel.md) and allocates storage for the
result. Calendar units also perform timezone-aware civil conversion per defined
element; fixed units use checked integer duration arithmetic. If the unit is
constant in a hot path, prefer the corresponding named method to avoid repeated
string normalization and dispatch.

## See Also

- [`add_days`](add_days.md): direct local-calendar day movement.
- [`add_hours`](add_hours.md): direct elapsed-hour movement.
- [`span`](span.md): measure the relationship between two timelines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_year.html"><span class="label">Previous</span><span class="title">set_year</span></a>
  <a class="next-link" href="sizes.html"><span class="label">Next</span><span class="title">sizes</span></a>
</div>
<!-- pluss-hand-authored -->
