<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ee3e920ba582 -->

# StreamTimeSeriesAccessor::calendar

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
pluss::table::DataFrame StreamTimeSeriesAccessor::calendar(const Stream& axis) const
```

## What It Does

Extracts calendar components from `axis` into their own columns: year,
month, day, hour, and a full weekday name — returned as a
`pluss::table::DataFrame`, one row per element of `axis`.

```{important} Ignores the bound stream's own value and kind entirely
Unlike every other method on this accessor, `calendar()` never reads the
bound stream's own values or requires it to be numeric — its output columns
depend only on the `axis` argument. Calling `notes.ts().calendar(axis)` on a
`Note`-kind `notes` stream succeeds exactly the same as on a numeric one.
```

```{important} Returns a DataFrame, not a Stream
Same escape from the kindle model as [`lag_block()`](lag_block.md)/
[`rolling_block()`](rolling_block.md).
```

## Parameters

**axis** (`const Stream&`)
    A `Timeline`/`TimeIndex` stream to extract calendar components from.

## Returns

`pluss::table::DataFrame`
    A new frame with five columns — `axis_year`, `axis_month`, `axis_day`,
    `axis_hour`, `axis_weekday` — one row per element of `axis`. The frame's
    case labels are `axis`'s own formatted timestamps.

## Examples

Example 1. Extract calendar components from an explicit `Timeline` axis.

```cpp
auto axis = pluss::kindle::Stream(pluss::make_timeline({
    "2024-03-14T09:30:00Z", "2024-12-25T00:00:00Z"}));
auto out = axis.ts().calendar(axis);
std::cout << out;
```

### Output

```text
==============================================================================
                    | axis_year  axis_month  axis_day  axis_hour  axis_weekday
------------------------------------------------------------------------------
2024-03-14 09:30:00 | 2024       3           14        9          Thursday    
2024-12-25 00:00:00 | 2024       12          25        0          Wednesday   
------------------------------------------------------------------------------
[2 cs x 5 feat]
```

`2024-03-14` is a Thursday and `2024-12-25` is a Wednesday; `axis_weekday`
here is the full English weekday name — a different rendering from
`DataFrame::ts().calendar()`'s `"YYYY-MM-DD (Abbreviated)"` format.

## Notes

- See the important callouts above.
- Columns are always prefixed `axis_`, regardless of what the `axis`
  argument stream is named or bound as.
- `axis_weekday` is the full English weekday name (`"Thursday"`), not an
  abbreviation and not a numeric code.

## See Also

- [`Stream::tl`](../stream/tl.md): the general calendar/timestamp accessor,
  offering `.year()`/`.month()` and similar individually rather than as one
  scribbled frame.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`DataFrameTsAccessor::calendar`](../dataframe-ts/calendar.md): the
  DataFrame-level equivalent, with a `time_feature`-by-name overload and a
  differently-formatted weekday column.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_to.html">
    <span class="label">Previous</span>
    <span class="title">align_to</span>
  </a>
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
