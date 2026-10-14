<!-- pluss-hand-authored -->

# SlateTimelineAccessor::month_start

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::month_start() const -> Slate
```

## What It Does

Replaces the bound slate with a boolean `Series` flagging whether each row
falls on the first day of its month.

## Parameters

`None`

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate:
    `true` when the day-of-month is `1`, `false` otherwise.

## Examples

Example 1. Flag month-start rows.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-06T14:00:00Z",
    "2023-03-01T00:00:00Z",
}));
auto out = events.tl().month_start();
std::cout << out;
```

### Output

```text
0  false
1  false
2  true

ptype: Series
```

Only `"2023-03-01"` falls on the first of its month.

## Notes

- Only checks day-of-month `== 1` — the time-of-day portion (if any) doesn't
  affect the result.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`, not `false`.

## See Also

- [`SlateTimelineAccessor::month_end`](month_end.md): the mirror-image check
  for the last day of the month.
- [`SlateTimelineAccessor::period`](period.md): truncate every row to its
  month start instead of just flagging it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="period.html">
    <span class="label">Next</span>
    <span class="title">period</span>
  </a>
</div>
