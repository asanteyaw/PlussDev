<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c0d8c936af1a -->

# DataFrameTimelineAccessor::iso

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::iso() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with a fixed
ISO-8601-with-offset text representation, as a `Note` `Series` — same feature
name, same row order. Equivalent to
[`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md).

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Note` Series
    holding each row as `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`.

## Examples

Example 1. Render every row of a `Timeline` feature as ISO-8601 text.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().iso();
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
=============================
     | ts                    
-----------------------------
0    | 2024-01-31T09:15:30.0~
1    | 2024-02-29T23:45:00.0~
2    | 2024-07-06T14:00:00.0~
3    | 2023-03-01T00:00:00.0~
-----------------------------
[4 cs x 1 feat]
```

Each row's full value is 35 characters long (e.g.
`"2024-01-31T09:15:30.000000000+00:00"`), but the table clips any cell wider
than 22 characters with a trailing `~` — the underlying `Note` values are
never actually truncated, only their display in this box render.

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed `%z`
  offset — there's no shorter variant; use [`fmt()`](fmt.md) with your own
  pattern for a more compact string.
- The offset is always `+HH:MM`/`-HH:MM` — UTC renders as `"+00:00"`, never a
  bare `"Z"`.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`.

## See Also

- [`DataFrameTimelineAccessor::fmt`](fmt.md): the general form this method is
  a fixed pattern of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
