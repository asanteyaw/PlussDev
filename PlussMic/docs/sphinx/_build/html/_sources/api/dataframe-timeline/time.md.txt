<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ac856538f117 -->

# DataFrameTimelineAccessor::time

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::time() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with just its
time-of-day portion — the date is reset to the Unix epoch day
(`1970-01-01`), and the clock time is kept, in local wall time — same
feature name, still `Timeline`/`TimeIndex`-kind, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature's date portion reset to
    `1970-01-01`, keeping only the time-of-day.

## Examples

Example 1. Keep only the time-of-day from every row of a `Timeline` feature.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().time();
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-02-29 23:45:00
2    | 2024-07-06 14:00:00
3    | 2023-03-01 00:00:00
--------------------------
[4 cs x 1 feat]
```

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 1970-01-01 09:15:30
1    | 1970-01-01 23:45:00
2    | 1970-01-01 14:00:00
3    | 1970-01-01 00:00:00
--------------------------
[4 cs x 1 feat]
```

Every row's date is now `1970-01-01`; the original time-of-day (`09:15:30`,
`23:45:00`, `14:00:00`, `00:00:00`) is preserved exactly.

## Notes

- The result is still a `Timeline`/`TimeIndex` (not a plain string or
  numeric type) with a fixed date component — it's meant for comparing or
  grouping by time-of-day, not as a display format. Use
  [`fmt("%H:%M:%S")`](fmt.md) if you want a bare text time.
- Operates in **local wall time**, per the `Timeline`'s current timezone.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`DataFrameTimelineAccessor::date`](date.md): the mirror-image operation,
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
