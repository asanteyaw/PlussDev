<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6aa6de554585 -->

# DataFrameTimelineAccessor::second

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::second() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with its
second-of-minute (`0`-`59`), as a numeric `Series` — same feature name, same
row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by its `Int64`
    second-of-minute.

## Examples

Example 1. Extract the second from every row of a `Timeline` feature.

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

auto out = events.tl().second();
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
===========
     | ts  
-----------
0    | 30  
1    | 0   
2    | 0   
3    | 0   
-----------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Int64`, holding each row's
second-of-minute. `second()` only reports whole seconds — sub-second
precision (nanoseconds) isn't exposed by any single `.tl()` field method; use
[`fmt("%f")`](fmt.md) or [`iso()`](iso.md) to see it as text.

## Notes

- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Thread`) displays as
  `"NULL"`.
- Reads the local second relative to the `Timeline`'s current timezone
  (seconds themselves are not affected by whole-hour/half-hour timezone
  offsets, but the row's identity as a second still follows local wall time).

## See Also

- [`DataFrameTimelineAccessor::minute`](minute.md),
  [`DataFrameTimelineAccessor::hour`](hour.md): the neighboring clock-time
  components.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
