<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 20042d2ff0d8 -->

# DataFrameTimelineAccessor::weekday_name

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::weekday_name() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with its full English
weekday name, as a `Note` (text) `Series` — same feature name, same row
order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Note` Series
    holding each row's full weekday name (`"Monday"`-`"Sunday"`).

## Examples

Example 1. Extract the weekday name from every row of a `Timeline` feature.

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

auto out = events.tl().weekday_name();
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
================
     | ts       
----------------
0    | Wednesday
1    | Thursday 
2    | Saturday 
3    | Wednesday
----------------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Note`-kind, holding each row's
full weekday name.

## Notes

- Always the full English name (`"Monday"`, not `"Mon"`); use
  [`fmt("%a")`](fmt.md) for the abbreviated three-letter form.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Thread`) displays as
  `"NULL"`, not a text placeholder like `"NaT"`.

## See Also

- [`DataFrameTimelineAccessor::weekday`](weekday.md): the same information as
  a `0`-`6` number instead of a name.
- [`DataFrameTimelineAccessor::month_name`](month_name.md): the equivalent for
  months.
- [`DataFrameTimelineAccessor::fmt`](fmt.md): build any other text
  representation via a custom pattern.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
