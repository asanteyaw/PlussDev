<!-- pluss-hand-authored -->

# ShelfTimelineAccessor::quarter

[Back to ShelfTimelineAccessor API Reference](../../shelf_timeline_accessor_api_reference)

## Signature

```cpp
Shelf ShelfTimelineAccessor::quarter() const
```

## What It Does

Replaces the bound shelf with its calendar quarter (`1`-`4`), as a numeric
`Series`.

## Parameters

`None`

## Returns

`Shelf`
    A new `Series`-kind (`Int64`) shelf, same length as the bound shelf,
    holding each row's quarter.

## Examples

Example 1. Extract the quarter from every row.

```cpp
using namespace pluss::table;

auto events = Shelf(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-06T14:00:00Z",
    "2023-03-01T00:00:00Z",
}));
auto out = events.tl().quarter();
std::cout << out;
```

### Output

```text
0  1
1  3
2  1

ptype: Series
```

January and March fall in Q1; July falls in Q3.

## Notes

- `.tl()` requires the bound shelf to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying shelf) displays as
  `"NULL"`.
- Calendar quarters only (`Jan-Mar`, `Apr-Jun`, `Jul-Sep`, `Oct-Dec`) — there's
  no fiscal-quarter offset parameter.

## See Also

- [`ShelfTimelineAccessor::month`](month.md): the underlying month
  `quarter()` is derived from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
