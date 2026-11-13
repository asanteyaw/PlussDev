<!-- pluss-hand-authored -->

# SlateTimelineAccessor::week

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
Slate SlateTimelineAccessor::week() const
```

## What It Does

Replaces the bound slate with its ISO-8601 week number (`1`-`53`), as a
numeric `Series`.

## Parameters

`None`

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate,
    holding each row's ISO week number.

## Examples

Example 1. Extract the ISO week number from every row.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().week();
std::cout << out;
```

### Output

```text
0  5
1  9
2  27

ptype: Series
```

## Notes

- Follows the ISO-8601 definition: weeks start on Monday, and week `1` is the
  week containing the year's first Thursday. Because of this, dates in very
  late December or very early January can report a week number that belongs
  to the neighboring calendar year — `week()` alone doesn't disambiguate which
  ISO year that week number belongs to.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`.

## See Also

- [`SlateTimelineAccessor::quarter`](quarter.md): the coarser calendar
  grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="valid.html">
    <span class="label">Previous</span>
    <span class="title">valid</span>
  </a>
  <a class="next-link" href="weekday.html">
    <span class="label">Next</span>
    <span class="title">weekday</span>
  </a>
</div>
