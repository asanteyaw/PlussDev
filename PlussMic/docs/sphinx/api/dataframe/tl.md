<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 298fee51fffc -->

# DataFrame::tl

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor DataFrame::tl() const
```

## What It Does

Enters the dedicated calendar/timestamp-operations accessor for the current
frame — extracting year, month, day, weekday, quarter, timezone conversion,
flooring/ceiling to a unit, and more, one method per operation. `tl()` itself does
no work beyond validating the frame and constructing the accessor: **every feature
currently on the frame must already be `Timeline`- or `TimeIndex`-kind**, or it
throws immediately with a message pointing at the fix (`"select timeline feature(s)
first, e.g. df.features({\"date\"}).tl()"`). In practice this means narrowing to the
timestamp feature(s) first, then calling `.tl()`:
`df.features({"ordered_at"}).tl().year()`, not `df.tl()` directly on a frame that
still has numeric or text columns.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)
and use its left sidebar to jump straight to any method (`year`, `month`, `weekday`,
`floor`, `tz_convert`, and more).

## Parameters

`tl()` itself takes no parameters. See each accessor method's own signature for what
it accepts.

## Returns

`DataFrameTimelineAccessor`
    A lightweight accessor bound to the current (all-Timeline/TimeIndex) frame.
    Every method on it returns a new `DataFrame` with the calendar computation
    applied, keeping each column's original feature name; the source frame is never
    mutated.

## Examples

Example 1. Extract the year from a timestamp feature — after selecting it first.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({1, 2, 3}))},
    {"ordered_at", Slate(pluss::make_timeline({
        "2024-01-02T00:00:00Z",
        "2024-02-20T00:00:00Z",
        "2024-03-10T00:00:00Z",
    }))},
});

auto years = orders.features({"ordered_at"}).tl().year();
std::cout << years;
```

### Output

```text
=================
     | ordered_at
-----------------
0    | 2024      
1    | 2024      
2    | 2024      
-----------------
[3 cs x 1 feat]
```

`orders.tl()` directly would have thrown, since `orders` still has a `Series`
feature (`order_id`) at the time of the call — `.features({"ordered_at"})` narrows
the frame down to just the timestamp column first, which is what `.tl()` requires.
The output column keeps the original feature name, `"ordered_at"`, now holding each
timestamp's year as an `Int64` value.

Example 2. Extract the month instead.

```cpp
auto months = orders.features({"ordered_at"}).tl().month();
std::cout << months;
```

### Output

```text
=================
     | ordered_at
-----------------
0    | 1         
1    | 2         
2    | 3         
-----------------
[3 cs x 1 feat]
```

`.month()` replaces each timestamp with its calendar month (`1`-`12`) as an `Int64`
value — `"2024-01-02..."` -> `1`, `"2024-02-20..."` -> `2`, `"2024-03-10..."` -> `3`.

## Notes

- Every feature on the frame `.tl()` is called on must already be `Timeline`- or
  `TimeIndex`-kind; select the timestamp feature(s) first with
  [`features()`](features.md) if the source frame has other kinds mixed in.
- The accessor's methods each apply to every feature on the (already-time-like)
  frame, returning a new frame with the transformation applied — some methods (like
  `.year()`, `.month()`, `.day()`) change the resulting kind from `Timeline` to a
  numeric `Series`, while others (like `.floor()`, `.utc()`) stay `Timeline`.
- `tl()` on a frame with no features at all, or a frame containing a feature that
  isn't Timeline/TimeIndex, throws before constructing the accessor.

## See Also

- [`DataFrame::features`](features.md): the usual way to narrow a frame down to just
  its timestamp features before calling `.tl()`.
- [`DataFrame::ts`](ts.md): the accessor for time-series *numeric* operations (lag,
  rolling windows, returns) on ordinary numeric features, distinct from this
  calendar-component accessor.
- [`DataFrame::nt`](nt.md): the equivalent dedicated accessor for `Note` text
  features, instead of timestamps.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="title_.html">
    <span class="label">Previous</span>
    <span class="title">title_</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
