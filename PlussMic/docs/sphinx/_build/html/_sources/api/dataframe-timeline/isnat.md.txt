<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ba84d325c7d3 -->

# DataFrameTimelineAccessor::isnat

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::isnat() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with a boolean `Series`
flagging whether the row is `NaT` (missing timestamp) — same feature name,
same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Bool` Series:
    `true` where the row is `NaT`, `false` otherwise.

## Examples

Example 1. Flag which rows of a `Timeline` feature are `NaT`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-10T08:00:00Z",
          "",
          "2024-03-05T17:30:00Z",
      }))}});
std::cout << events;

auto out = events.tl().isnat();
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
0    | false
1    | true 
2    | false
------------
[3 cs x 1 feat]
```

Row `1` — the missing timestamp displayed as `"NULL"` above — flags `true`
here.

## Notes

- **`isnat()` is one of the few `.tl()` methods whose own output is never
  masked to `"NULL"`** — the whole point of the method is to answer "is this
  row missing," so the row that *is* `NaT` shows `true`, not `NULL`. Nearly
  every other `.tl()` method (the calendar-field extractors, `fmt()`,
  `epoch()`, and so on) instead shows `"NULL"` for a `NaT` row.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.

## See Also

- [`DataFrameTimelineAccessor::valid`](valid.md): the logical inverse (with
  the same never-`NULL` behavior).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
