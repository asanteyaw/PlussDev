<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ede01210505b -->

# DataFrameTimelineAccessor::utc

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::utc() const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature's display timezone
with `"UTC"` — same feature name, still `Timeline`/`TimeIndex`-kind, same row
order. Exactly equivalent to [`tz("UTC")`](tz.md); like `tz()`, the
underlying instant never changes, only the timezone used to display it and
compute local calendar fields.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature's display timezone switched
    to `"UTC"`.

## Examples

Example 1. Convert a `"America/New_York"`-labeled `Timeline` feature back to
UTC display.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-07-15T09:15:30Z",
      }))}});
auto ny = events.tl().tz("America/New_York");
std::cout << ny;

auto out = ny.tl().utc();
std::cout << out;
```

### Output (before, displayed in America/New_York)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 04:15:30
1    | 2024-07-15 05:15:30
--------------------------
[2 cs x 1 feat]
```

### Output (after utc())

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-07-15 09:15:30
--------------------------
[2 cs x 1 feat]
```

`utc()` undoes exactly what the earlier `tz("America/New_York")` call did —
same instants throughout, only the display timezone (and therefore the
printed wall-clock text) changes back.

## Notes

- Purely a display-timezone relabel — the underlying instant is unchanged, so
  chaining `.tz(name).utc()` always round-trips back to the original
  displayed values.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT`.

## See Also

- [`DataFrameTimelineAccessor::tz`](tz.md): the general form this method is a
  fixed `"UTC"` shorthand for.
- [`DataFrameTimelineAccessor::localize`](localize.md): the operation that
  actually changes the underlying instant, unlike this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html">
    <span class="label">Previous</span>
    <span class="title">tz</span>
  </a>
  <a class="next-link" href="valid.html">
    <span class="label">Next</span>
    <span class="title">valid</span>
  </a>
</div>
