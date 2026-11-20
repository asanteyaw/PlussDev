<!-- pluss-hand-authored -->

# ShelfTimelineAccessor::second

[Back to ShelfTimelineAccessor API Reference](../../shelf_timeline_accessor_api_reference)

## Signature

```cpp
Shelf ShelfTimelineAccessor::second() const
```

## What It Does

Replaces the bound shelf with its second-of-minute (`0`-`59`), as a numeric
`Series`.

## Parameters

`None`

## Returns

`Shelf`
    A new `Series`-kind (`Int64`) shelf, same length as the bound shelf,
    holding each row's second-of-minute.

## Examples

Example 1. Extract the second from every row.

```cpp
using namespace pluss::table;

auto events = Shelf(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().second();
std::cout << out;
```

### Output

```text
0  30
1  0
2  0

ptype: Series
```

`second()` only reports whole seconds — sub-second precision (nanoseconds)
isn't exposed by any single `.tl()` field method; use [`fmt("%f")`](fmt.md) or
[`iso()`](iso.md) to see it as text.

## Notes

- `.tl()` requires the bound shelf to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying shelf) displays as
  `"NULL"`.
- Reads the local second relative to the shelf's current timezone.

## See Also

- [`ShelfTimelineAccessor::minute`](minute.md),
  [`ShelfTimelineAccessor::hour`](hour.md): the neighboring clock-time
  components.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
