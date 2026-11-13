<!-- pluss-hand-authored -->

# SlateTimelineAccessor::yearday

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
Slate SlateTimelineAccessor::yearday() const
```

## What It Does

Replaces the bound slate with its 1-based day-of-year (`1`-`366`), as a
numeric `Series`.

## Parameters

`None`

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate,
    holding each row's day-of-year.

## Examples

Example 1. Extract the day-of-year from every row, including a leap-day and
non-leap-year contrast.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
    "2023-03-01T00:00:00Z",
}));
auto out = events.tl().yearday();
std::cout << out;
```

### Output

```text
0  60
1  188
2  60

ptype: Series
```

`"2024-02-29"` (a leap day) and `"2023-03-01"` (a non-leap year) both land on
day `60` — 2024's leap day shifts everything from March onward by one day
relative to a non-leap year, so early-March dates coincide numerically.

## Notes

- 1-based: January 1st is day `1`, never `0`.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`.

## See Also

- [`SlateTimelineAccessor::leap`](leap.md): whether the row's year is a leap
  year, which is exactly what shifts `yearday()` by one day after February.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
</div>
