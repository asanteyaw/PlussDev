<!-- pluss-hand-authored -->

# ShelfTimelineAccessor::date

[Back to ShelfTimelineAccessor API Reference](../../shelf_timeline_accessor_api_reference)

## Signature

```cpp
Shelf ShelfTimelineAccessor::date() const
```

## What It Does

Replaces the bound shelf with just its date portion (time-of-day dropped to
midnight), in local wall time — same length, still `Timeline`-kind.
Equivalent to [`floor("D")`](floor.md).

## Parameters

`None`

## Returns

`Shelf`
    A new `Timeline`-kind shelf, same length as the bound shelf,
    with its clock-time portion zeroed out.

## Examples

Example 1. Strip the time-of-day from every row.

```cpp
using namespace pluss::table;

auto events = Shelf(pluss::make_timeline({
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

- Operates in **local wall time**, per the shelf's current timezone.
- `.tl()` requires the bound shelf to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`ShelfTimelineAccessor::time`](time.md): the mirror-image operation,
  keeping the time-of-day and zeroing the date.
- [`ShelfTimelineAccessor::floor`](floor.md): the general form `date()` is a
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
