<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fec323a6fc1d -->

# DataFrameTimelineAccessor::valid

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::valid() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with a boolean `Series`
flagging whether the row is a real (non-`NaT`) timestamp — the logical
inverse of [`isnat()`](isnat.md) — same feature name, same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Bool` Series:
    `true` where the row is a real timestamp, `false` where it's `NaT`.

## Examples

Example 1. Flag which rows of a `Timeline` feature are real timestamps.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-10T08:00:00Z",
          "",
          "2024-03-05T17:30:00Z",
      }))}});
std::cout << events;

auto out = events.tl().valid();
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-10 08:00:00
1    | NULL               
2    | 2024-03-05 17:30:00
--------------------------
[3 cs x 1 feat]
```

### Output (after)

```text
============
     | ts   
------------
0    | true 
1    | false
2    | true 
------------
[3 cs x 1 feat]
```

Row `1` — the missing timestamp — flags `false` here.

## Notes

- **`valid()` is one of the few `.tl()` methods whose own output is never
  masked to `"NULL"`** — it exists precisely to answer "is this row real,"
  so the `NaT` row shows `false`, not `NULL`. Nearly every other `.tl()`
  method (the calendar-field extractors, `fmt()`, `epoch()`, and so on)
  instead shows `"NULL"` for a `NaT` row.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.

## See Also

- [`DataFrameTimelineAccessor::isnat`](isnat.md): the logical inverse (with
  the same never-`NULL` behavior).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="utc.html">
    <span class="label">Previous</span>
    <span class="title">utc</span>
  </a>
  <a class="next-link" href="week.html">
    <span class="label">Next</span>
    <span class="title">week</span>
  </a>
</div>
