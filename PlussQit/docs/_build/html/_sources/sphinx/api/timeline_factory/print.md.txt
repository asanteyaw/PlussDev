<!-- pluss-hand-authored -->
# pluss::timeline::print

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
std::string pluss::timeline::print(const Timeline& value)
std::string pluss::timeline::print(const Timeline& value,
                                   const TimelinePrintOptions& options)
std::ostream& pluss::timeline::print(std::ostream& out,
                                    const Timeline& value,
                                    const TimelinePrintOptions& options = {})
```

## Render A Timeline For A Human Reader

`print()` produces Timeline's structural diagnostic representation: optional
type, shape, and timezone metadata followed by values laid out according to the
rank. It is intended for terminals, logs, examples, and debugging. The string
overloads return the complete rendering; the stream overload writes the same
rendering directly and returns the stream so normal stream composition works.

This is different from `format()`. `format()` returns a shape-preserving
`Note`, with one independently usable string for every instant. `print()`
returns or writes one presentation string for the Timeline as a whole. A
formatted result is therefore printed with `pluss::note::print()`, while the
Timeline itself is printed with `pluss::timeline::print()`.

The default rendering shows whole seconds. It is not a serialization format
and should not be used to prove nanosecond identity. Select `%f` explicitly or
inspect `epoch_nanos()` when subsecond precision matters.

## Layout Options

`TimelinePrintOptions` controls presentation without modifying the value:

| Field | Default | Purpose |
| --- | --- | --- |
| `format` | `%Y-%m-%d %H:%M:%S` | text for each visible instant |
| `edge_items` | `3` | number retained at each truncated edge |
| `max_rows` | `10` | row threshold for rank-two and rank-three values |
| `max_cols` | `7` | column threshold for rank-one through rank-three values |
| `max_slices` | `6` | outer-slice threshold for rank-three values |
| `show_header` | `true` | include the `Timeline(...)` header |
| `show_shape` | `true` | include shape in that header |
| `show_tz` | `true` | include timezone in that header |

When an axis exceeds its threshold, the renderer keeps its leading and
trailing `edge_items` and inserts `...`. Truncation changes only the text; it
does not slice the Timeline or change `numel()`.

## Example 1: Default Structural Rendering

```cpp
const auto values = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z",
    "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z",
    "2024-01-05T00:00:00Z", "2024-01-06T00:00:00Z"});

std::cout << pluss::timeline::print(values) << '\n';
```

### Output

```text
Timeline(shape=[6], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-02 00:00:00, 2024-01-03 00:00:00, 2024-01-04 00:00:00, 2024-01-05 00:00:00, 2024-01-06 00:00:00 ]
```

## Example 2: Bound A Long Axis

```cpp
auto options = pluss::TimelinePrintOptions{};
options.format = "%F";
options.max_cols = 4;
options.edge_items = 1;
options.show_tz = false;

std::cout << pluss::timeline::print(values, options) << '\n';
```

### Output

```text
Timeline(shape=[6])
[ 2024-01-01, ..., 2024-01-06 ]
```

The source still has six elements. The ellipsis is a rendering marker, not a
Timeline element.

## Example 3: Expose Exact Nanoseconds Without A Header

```cpp
auto options = pluss::TimelinePrintOptions{};
options.format = "%FT%T.%f%z";
options.show_header = false;

const auto one_nanosecond = pluss::timeline::from_epoch(1LL);
std::cout << pluss::timeline::print(one_nanosecond, options) << '\n';
```

### Output

```text
1970-01-01T00:00:00.000000001+00:00
```

## Example 4: Write Directly To An Existing Stream

```cpp
std::ostringstream buffer;
const auto stream_values = pluss::timeline::date(
    {"2024-01-01", "2024-01-02"});
auto options = pluss::TimelinePrintOptions{};
options.format = "%F";
options.show_header = false;

auto& returned = pluss::timeline::print(buffer, stream_values, options);
returned << "\nrecords=" << stream_values.numel();

std::cout << (&returned == &buffer) << '\n';
std::cout << buffer.str() << '\n';
```

### Output

```text
true
[ 2024-01-01, 2024-01-02 ]
records=2
```

The overload returns the same stream. This avoids a caller-side temporary
string when output is already headed to a file, socket, or logging stream.

## Failures And Cost

An unsupported format directive throws rather than being silently copied.
Formatting currently visits every lane before the layout stage chooses visible
edges, so rendering cost remains linear in `numel()` even when the displayed
text is truncated. The returned-string overload necessarily allocates its
output; use the stream overload when direct emission is preferable.

## See Also

- [`format`](format.md): return a shape-preserving Note for further text work.
- [`from_epoch`](from_epoch.md): inspect exact stored nanoseconds when identity matters.
