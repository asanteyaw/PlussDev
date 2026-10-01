<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::week

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::week() const -> Thread
```

## What It Does

Replaces the bound thread with its ISO-8601 week number (`1`-`53`), as a
numeric `Series`.

## Parameters

`None`

## Returns

`Thread`
    A new `Series`-kind (`Int64`) thread, same length as the bound thread,
    holding each row's ISO week number.

## Examples

Example 1. Extract the ISO week number from every row.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
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
- `.tl()` requires the bound thread to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying thread) displays as
  `"NULL"`.

## See Also

- [`ThreadTimelineAccessor::quarter`](quarter.md): the coarser calendar
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
