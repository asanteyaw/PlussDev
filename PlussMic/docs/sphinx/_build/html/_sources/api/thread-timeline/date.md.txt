<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::date

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::date() const -> Thread
```

## What It Does

Replaces the bound thread with just its date portion (time-of-day dropped to
midnight), in local wall time — same length, still `Timeline`/`TimeIndex`-kind.
Equivalent to [`floor("D")`](floor.md).

## Parameters

`None`

## Returns

`Thread`
    A new `Timeline`/`TimeIndex`-kind thread, same length as the bound thread,
    with its clock-time portion zeroed out.

## Examples

Example 1. Strip the time-of-day from every row.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().date();
std::cout << out;
```

### Output

```text
0  2024-01-31
1  2024-02-29
2  2024-07-06

ptype: Timeline
```

Every row's clock-time portion is now `00:00:00`, so the whole column displays
without any time-of-day text at all.

## Notes

- Operates in **local wall time**, per the thread's current timezone.
- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`ThreadTimelineAccessor::time`](time.md): the mirror-image operation,
  keeping the time-of-day and zeroing the date.
- [`ThreadTimelineAccessor::floor`](floor.md): the general form `date()` is a
  fixed `"D"` unit of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="day.html">
    <span class="label">Next</span>
    <span class="title">day</span>
  </a>
</div>
