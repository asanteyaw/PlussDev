<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::day

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::day() const -> Thread
```

## What It Does

Replaces the bound thread with its day-of-month (`1`-`31`), as a numeric
`Series`.

## Parameters

`None`

## Returns

`Thread`
    A new `Series`-kind (`Int64`) thread, same length as the bound thread,
    holding each row's day-of-month.

## Examples

Example 1. Extract the day-of-month from every row.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().day();
std::cout << out;
```

### Output

```text
0  31
1  29
2  6

ptype: Series
```

## Notes

- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying thread) displays as
  `"NULL"`.
- Reads the local calendar day relative to the thread's current timezone.

## See Also

- [`ThreadTimelineAccessor::year`](year.md),
  [`ThreadTimelineAccessor::month`](month.md): the other calendar-date
  components.
- [`ThreadTimelineAccessor::month_start`](month_start.md),
  [`ThreadTimelineAccessor::month_end`](month_end.md): whether this day is its
  month's first/last.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date.html">
    <span class="label">Previous</span>
    <span class="title">date</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
