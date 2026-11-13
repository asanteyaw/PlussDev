<!-- pluss-hand-authored -->

# SlateTimelineAccessor::time

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
Slate SlateTimelineAccessor::time() const
```

## What It Does

Replaces the bound slate with just its time-of-day portion — the date is
reset to the Unix epoch day (`1970-01-01`), and the clock time is kept, in
local wall time — same length, still `Timeline`-kind.

## Parameters

`None`

## Returns

`Slate`
    A new `Timeline`-kind slate, same length as the bound
    slate, with its date portion reset to `1970-01-01`, keeping only the
    time-of-day.

## Examples

Example 1. Keep only the time-of-day from every row.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().time();
std::cout << out;
```

### Output

```text
0  1970-01-01 09:15:30
1  1970-01-01 23:45:00
2  1970-01-01 14:00:00

ptype: Timeline
```

Every row's date is now `1970-01-01`; the original time-of-day is preserved
exactly.

## Notes

- The result is still a `Timeline` (not a plain string or numeric
  type) with a fixed date component — it's meant for comparing or grouping by
  time-of-day, not as a display format. Use [`fmt("%H:%M:%S")`](fmt.md) if a
  bare text time is needed.
- Operates in **local wall time**, per the slate's current timezone.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`SlateTimelineAccessor::date`](date.md): the mirror-image operation,
  keeping the date and zeroing the time-of-day.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="span.html">
    <span class="label">Previous</span>
    <span class="title">span</span>
  </a>
  <a class="next-link" href="tz.html">
    <span class="label">Next</span>
    <span class="title">tz</span>
  </a>
</div>
