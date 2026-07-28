<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::hour

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::hour() const -> Thread
```

## What It Does

Replaces the bound thread with its hour-of-day (`0`-`23`), as a numeric
`Series`.

```{note} Also available as a direct Thread method
`Thread::hour()` exists directly (no `.tl()` needed) and produces the
identical result.
```

## Parameters

`None`

## Returns

`Thread`
    A new `Series`-kind (`Int64`) thread, same length as the bound thread,
    holding each row's hour-of-day.

## Examples

Example 1. Extract the hour from every row.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
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

- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying thread) displays as
  `"NULL"`.
- Reads the local hour relative to the thread's current timezone; changing
  timezone with [`tz()`](tz.md) changes what `hour()` reports for the same
  underlying instant.
- Identical to calling [`Thread::hour()`](../thread/hour.md) directly.

## See Also

- [`ThreadTimelineAccessor::minute`](minute.md),
  [`ThreadTimelineAccessor::second`](second.md): the finer clock-time
  components.
- [`ThreadTimelineAccessor::time`](time.md): the full clock-time portion as a
  `Timeline`, instead of one numeric field at a time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
