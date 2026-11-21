<!-- pluss-hand-authored -->

# ShelfCastAccessor::duration

[Back to ShelfCastAccessor API Reference](../../shelf_cast_accessor_api_reference)

## Signature

```cpp
Shelf ShelfCastAccessor::duration() const
```

## What It Does

Converts the bound shelf to a `Duration`-kind shelf: elapsed time stored as signed nanoseconds. Which
conversions are accepted is deliberately narrow, because a count of nanoseconds is only meaningful when it
is exact:

- a `Duration` shelf is returned unchanged;
- a `Series` shelf must have the `Int64` dtype, and each value is read as signed nanoseconds;
- a `Note` shelf must hold exact signed nanoseconds written as decimal text.

Calling `duration()` on a `Timeline` shelf throws, because a timestamp is a point in time, not an elapsed
length. `ShelfCastOptions::format` is not accepted here and throws if it is set.

## Returns

`Shelf`
    A new `Duration`-kind shelf with the same length and the same `NULL` positions as the bound shelf.

## Examples

Example 1. Read an `Int64` series as signed nanoseconds.

```cpp
using namespace pluss::table;

auto ticks = Shelf(pluss::make_series(std::vector<std::int64_t>{1500000000, 60000000000}, {2}));

std::cout << ticks.cast().duration();
```

### Output

```text
0  PT1.5S
1  PT1M
ptype: Duration
```

Example 2. Read decimal nanosecond text, including a negative value.

```cpp
auto text = Shelf(pluss::make_note({"1500000000", "-250"}));

std::cout << text.cast().duration();
```

### Output

```text
0  PT1.5S
1  -PT0.00000025S
ptype: Duration
```

Example 3. Text that is not an exact nanosecond count throws by default. With `invalid = ParseInvalid::Null`
the bad row becomes `NULL` instead.

```cpp
auto messy = Shelf(pluss::make_note({"1500000000", "soon"}));

try {
  auto out = messy.cast().duration();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}

ShelfCastOptions null_on_invalid;
null_on_invalid.invalid = ParseInvalid::Null;
std::cout << messy.cast(null_on_invalid).duration();
```

### Output

```text
Shelf::cast(Duration): could not parse exact signed nanoseconds from 'soon'
0  PT1.5S
1  NULL
ptype: Duration
```

Example 4. A series that is not `Int64`, and a `format`, are both refused.

```cpp
try {
  auto out = Shelf(pluss::make_series({1.5, 2.5})).cast().duration();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}

ShelfCastOptions with_format;
with_format.format = "%S";
try {
  auto out = Shelf(pluss::make_note({"1"})).cast(with_format).duration();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
Shelf::cast(Duration): Series source must have Int64 dtype
Shelf::cast(Duration): format is not accepted; text must contain exact signed nanoseconds
```

## Notes

- The Series dtype is checked, not converted: cast a floating-point series to `Int64` first (with
  [`Shelf::astype`](../shelf/astype.md)) if its values are whole nanoseconds.
- `NULL` rows stay `NULL` and are never parsed.

## See Also

- [`Shelf::cast`](../shelf/cast.md): enters the accessor and supplies the `ShelfCastOptions`.
- [`ShelfCastAccessor::series`](series.md): the reverse direction, `Duration` to nanosecond integers.
- [`ShelfCastAccessor::timeline`](timeline.md): the point-in-time target.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>
