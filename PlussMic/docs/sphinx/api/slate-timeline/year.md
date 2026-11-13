<!-- pluss-hand-authored -->

# SlateTimelineAccessor::year

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
Slate SlateTimelineAccessor::year() const
```

## What It Does

Replaces the bound slate with its calendar year, as a numeric `Series`.

## Parameters

`None`

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate,
    holding each row's calendar year.

## Examples

Example 1. Extract the year from every row.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-06T14:00:00Z",
    "2023-03-01T00:00:00Z",
}));
auto out = events.tl().year();
std::cout << out;
```

### Output

```text
0  2024
1  2024
2  2023

ptype: Series
```

## Notes

- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A row whose underlying timestamp is `NaT` (or whose slate already carries a
  null mask) displays as `"NULL"` in the year output, not some sentinel
  number.
- `year()` reads local calendar fields — i.e. relative to whatever timezone
  the slate currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).

## See Also

- [`SlateTimelineAccessor::month`](month.md),
  [`SlateTimelineAccessor::day`](day.md): the other calendar-date
  components.
- [`SlateTimelineAccessor::leap`](leap.md): whether the row's year is a leap
  year.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekend.html">
    <span class="label">Previous</span>
    <span class="title">weekend</span>
  </a>
  <a class="next-link" href="yearday.html">
    <span class="label">Next</span>
    <span class="title">yearday</span>
  </a>
</div>
