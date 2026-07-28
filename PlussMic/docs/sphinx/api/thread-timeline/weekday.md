<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::weekday

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::weekday() const -> Thread
```

## What It Does

Replaces the bound thread with its day-of-week, as a numeric `Series`.
Encoding is `0 = Monday` through `6 = Sunday`.

## Parameters

`None`

## Returns

`Thread`
    A new `Series`-kind (`Int64`) thread, same length as the bound thread,
    holding each row's day-of-week (`0`-`6`, Monday-first).

## Examples

Example 1. Extract the day-of-week from every row.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().weekday();
std::cout << out;
```

### Output

```text
0  2
1  3
2  5

ptype: Series
```

`"2024-01-31"` is a Wednesday (`2`); `"2024-02-29"` is a Thursday (`3`);
`"2024-07-06"` is a Saturday (`5`).

## Notes

- The encoding is Monday `0` through Sunday `6` — **not** the C/Python
  `tm_wday`-style Sunday-first convention some other libraries use.
- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying thread) displays as
  `"NULL"`.

## See Also

- [`ThreadTimelineAccessor::weekday_name`](weekday_name.md): the same
  information as a text name instead of a number.
- [`ThreadTimelineAccessor::weekend`](weekend.md): whether the day falls on
  Saturday/Sunday.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html">
    <span class="label">Previous</span>
    <span class="title">week</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
