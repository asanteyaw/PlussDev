<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 16314017416e -->

# DataFrameTimelineAccessor::date

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::date() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with just its date
portion (time-of-day dropped to midnight), in local wall time — same feature
name, still `Timeline`/`TimeIndex`-kind, same row order. Equivalent to
[`floor("D")`](floor.md).

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature's clock-time portion zeroed
    out.

## Examples

Example 1. Strip the time-of-day from every row of a `Timeline` feature.

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

auto out = events.tl().date();
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
=================
     | ts        
-----------------
0    | 2024-01-31
1    | 2024-02-29
2    | 2024-07-06
3    | 2023-03-01
-----------------
[4 cs x 1 feat]
```

Every row's clock-time portion is now `00:00:00`, so the whole column
displays without any time-of-day text at all.

## Notes

- Operates in **local wall time**, per the `Timeline`'s current timezone.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` (displays as `"NULL"`).

## See Also

- [`DataFrameTimelineAccessor::time`](time.md): the mirror-image operation,
  keeping the time-of-day and zeroing the date.
- [`DataFrameTimelineAccessor::floor`](floor.md): the general form
  `date()` is a fixed `"D"` unit of.

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
