<!-- pluss-hand-authored -->

# ShelfCastAccessor::note

[Back to ShelfCastAccessor API Reference](../../shelf_cast_accessor_api_reference)

## Signature

```cpp
Shelf ShelfCastAccessor::note() const
```

## What It Does

Converts the bound shelf to a `Note`-kind shelf, that is, to text:

- a `Note` shelf is returned unchanged;
- a `Series` shelf is formatted value by value, each number written as its own text;
- a `Timeline` shelf is written as ISO timestamps;
- a `Duration` shelf is written as ISO-8601 durations.

Every source kind can be converted, so this method has no error cases of its own.

## Returns

`Shelf`
    A new `Note`-kind shelf with the same length and the same `NULL` positions as the bound shelf.

## Examples

Example 1. Format a numeric shelf as text.

```cpp
using namespace pluss::table;

auto amounts = Shelf(pluss::make_series({1.5, 2.5}));

std::cout << amounts.cast().note();
```

### Output

```text
0  1.5
1  2.5
ptype: Note
```

Example 2. Format a `Duration` shelf as ISO-8601 durations.

```cpp
auto lengths = Shelf(pluss::duration::nanos(std::vector<std::int64_t>{1500000000}, {1}));

std::cout << lengths.cast().note();
```

### Output

```text
0  PT1.5S
ptype: Note
```

Example 3. Format a `Timeline` shelf as ISO timestamps.

```cpp
auto stamps = Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z"}));

std::cout << stamps.cast().note();
```

### Output

```text
0  2024-01-01T00:00:00.0~
ptype: Note
```

The printed table shortens long cells with `~`; the stored text is the full ISO timestamp.

## Notes

- Converting a shelf that is already a `Note` is a cheap identity.
- `NULL` rows stay `NULL`.

## See Also

- [`Shelf::cast`](../shelf/cast.md): enters the accessor and supplies the `ShelfCastOptions`.
- [`ShelfCastAccessor::series`](series.md), [`timeline`](timeline.md),
  [`duration`](duration.md): the other targets.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duration.html">
    <span class="label">Previous</span>
    <span class="title">duration</span>
  </a>
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
