<!-- pluss-hand-authored -->

# ShelfCastAccessor::timeline

[Back to ShelfCastAccessor API Reference](../../shelf_cast_accessor_api_reference)

## Signature

```cpp
Shelf ShelfCastAccessor::timeline() const
```

## What It Does

Converts the bound shelf to a `Timeline`-kind shelf, that is, to points in time:

- a `Timeline` shelf is returned unchanged;
- a numeric `Series` shelf is read as epoch nanoseconds, or as calendar dates when every valid value looks
  like an eight-digit `YYYYMMDD` integer;
- a `Note` shelf is parsed as ISO-style date and time text, or with `options.format` when that is set.

A `Duration` shelf is refused, because elapsed time is not a timestamp.

## Parameters

The accessor takes the `ShelfCastOptions` supplied to [`Shelf::cast`](../shelf/cast.md).
`format` is a strptime-style pattern for `Note` text. `invalid` chooses what an unparseable `Note` value does:
`Raise` (the default) throws, and `Null` marks the row `NULL`.

## Returns

`Shelf`
    A new `Timeline`-kind shelf with the same length as the bound shelf.

## Examples

Example 1. Parse ISO text.

```cpp
using namespace pluss::table;

auto text = Shelf(pluss::make_note({"2024-01-01T00:00:00Z", "2024-06-30T12:30:00Z"}));

std::cout << text.cast().timeline();
```

### Output

```text
0  2024-01-01 00:00:00
1  2024-06-30 12:30:00
ptype: Timeline
```

Example 2. Read eight-digit `YYYYMMDD` integers as dates, and a single large integer as epoch nanoseconds.

```cpp
auto dates = Shelf(pluss::make_series(std::vector<std::int64_t>{20240101, 20240229}, {2}));
auto epoch = Shelf(pluss::make_series(std::vector<std::int64_t>{1704067200000000000LL}, {1}));

std::cout << dates.cast().timeline();
std::cout << epoch.cast().timeline();
```

### Output

```text
0  2024-01-01
1  2024-02-29
ptype: Timeline
0  2024-01-01
ptype: Timeline
```

Example 3. Parse text in your own layout with `format`.

```cpp
ShelfCastOptions options;
options.format = "%d/%m/%Y";

auto text = Shelf(pluss::make_note({"31/12/2024", "01/01/2025"}));
std::cout << text.cast(options).timeline();
```

### Output

```text
0  2024-12-31
1  2025-01-01
ptype: Timeline
```

Example 4. Text that is not a date throws by default. With `invalid = ParseInvalid::Null` the bad row
becomes `NULL` instead.

```cpp
auto messy = Shelf(pluss::make_note({"2024-01-01T00:00:00Z", "not a date"}));

try {
  auto out = messy.cast().timeline();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}

ShelfCastOptions null_on_invalid;
null_on_invalid.invalid = ParseInvalid::Null;
std::cout << messy.cast(null_on_invalid).timeline();
```

### Output

```text
moment: failed to parse datetime
0  2024-01-01
1  NULL
ptype: Timeline
```

Example 5. A `Duration` shelf is refused.

```cpp
auto lengths = Shelf(pluss::duration::nanos(std::vector<std::int64_t>{5}, {1}));

try {
  auto out = lengths.cast().timeline();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
Shelf::cast(Timeline): Duration is elapsed time, not an epoch timestamp
```

## Notes

- A numeric `Series` is treated as dates only if every valid value is a whole number from 10000101 to
  99991231 and forms a real calendar date. Otherwise all values are read as epoch nanoseconds.
- Converting a shelf that is already a `Timeline` is a cheap identity.

## See Also

- [`Shelf::cast`](../shelf/cast.md): enters the accessor and supplies the `ShelfCastOptions`.
- [`ShelfCastAccessor::series`](series.md): the reverse direction, `Timeline` to epoch nanoseconds.
- [`ShelfCastAccessor::note`](note.md), [`duration`](duration.md): the other targets.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
</div>
