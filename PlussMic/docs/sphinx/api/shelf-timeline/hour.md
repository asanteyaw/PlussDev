<!-- pluss-hand-authored -->

# ShelfTimelineAccessor::hour

[Back to ShelfTimelineAccessor API Reference](../../shelf_timeline_accessor_api_reference)

## Signature

```cpp
Shelf ShelfTimelineAccessor::hour() const
```

## What It Does

Replaces the bound shelf with its hour-of-day (`0`-`23`), as a numeric
`Series`.

## Parameters

`None`

## Returns

`Shelf`
    A new `Series`-kind (`Int64`) shelf, same length as the bound shelf,
    holding each row's hour-of-day.

## Examples

Example 1. Extract the hour from every row.

```cpp
using namespace pluss::table;

auto events = Shelf(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().hour();
std::cout << out;
```

### Output

```text
0  9
1  23
2  14

ptype: Series
```

## Notes

- `.tl()` requires the bound shelf to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying shelf) displays as
  `"NULL"`.
- Reads the local hour relative to the shelf's current timezone; changing
  timezone with [`tz()`](tz.md) changes what `hour()` reports for the same
  underlying instant.

## See Also

- [`ShelfTimelineAccessor::minute`](minute.md),
  [`ShelfTimelineAccessor::second`](second.md): the finer clock-time
  components.
- [`ShelfTimelineAccessor::time`](time.md): the full clock-time portion as a
  `Timeline`, instead of one numeric field at a time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
