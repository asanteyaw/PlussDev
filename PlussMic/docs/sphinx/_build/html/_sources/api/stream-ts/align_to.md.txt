<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f55c8c75513 -->

# StreamTimeSeriesAccessor::align_to

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signatures

```cpp
Stream StreamTimeSeriesAccessor::align_to(
    const Stream& source_axis, const Stream& target_axis,
    pluss::table::TsAlignMode mode = pluss::table::TsAlignMode::Exact) const

Stream StreamTimeSeriesAccessor::align_to(
    const Stream& source_axis, const Stream& target_axis,
    pluss::table::TsAlignMode mode,
    pluss::subsp::stats::ts::GapFillMode fill_mode,
    std::optional<double> constant = std::nullopt) const
```

## What It Does

Reindexes the bound stream's values — currently living at `source_axis`
ticks — onto an entirely new `target_axis`, matching each target tick to a
`source_axis` tick according to `mode`: `Exact` (the default, matches only
identical ticks), `Backward` (last known value), `Forward` (next known
value), or `Nearest` (closest tick either direction, ties toward earlier).

The `fill_mode`/`constant` overload additionally runs
[`fill_gaps(fill_mode, constant)`](fill_gaps.md) on the result afterward.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**source_axis** (`const Stream&`)
    The time axis (`Timeline`/`TimeIndex`) the bound stream's own values
    currently live at. Must be the same length as the bound stream.

**target_axis** (`const Stream&`)
    The new time axis to reindex onto. Determines the result's length.

**mode** (`pluss::table::TsAlignMode`, default `Exact`)
    The matching strategy described above.

**fill_mode** (`pluss::subsp::stats::ts::GapFillMode`, fill overload)
    The gap-fill strategy applied after alignment.

**constant** (`std::optional<double>`, fill overload)
    The fill value when `fill_mode` is `Constant`. Required in that case.

## Returns

`Stream`
    A new stream, length `target_axis.size()`, holding the bound values
    reindexed according to `mode` (and optionally gap-filled).

## Examples

Example 1. Reindex a sparse stream onto a regular daily grid using `Exact`
matching.

```cpp
auto source_axis = pluss::kindle::Stream(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"}));
auto prices = pluss::kindle::Stream(pluss::make_series({100.0, 102.0, 110.0}));
auto grid = pluss::kindle::Stream(pluss::make_timeline({
    "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z",
    "2024-01-04T00:00:00Z", "2024-01-05T00:00:00Z"}));
auto out = prices.ts().align_to(source_axis, grid, pluss::table::TsAlignMode::Exact);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=5
label=yes  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
2024-01-01T~ | 100                                                                                                      
2024-01-02T~ | 102                                                                                                      
2024-01-03T~ | NA                                                                                                       
2024-01-04T~ | NA                                                                                                       
2024-01-05T~ | 110                                                                                                      
========================================================================================================================
```

`Exact` only fills in the grid ticks that have an exact match in
`source_axis`; the ticks with no source row at all are `NULL`.

## Notes

- `source_axis` must be the same length as the bound stream.
- `Nearest` breaks a tie between two equally-close source ticks toward the
  *earlier* one.
- The result's length always matches `target_axis.size()`, regardless of the
  bound stream's own length.
- See the important callout above.

## See Also

- [`StreamTimeSeriesAccessor::fill_gaps`](fill_gaps.md): the gap-fill step
  the `fill_mode`/`constant` overload applies automatically.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::align_to`](../thread-ts/align_to.md): the underlying
  single-thread engine, with the full comparison of all four `mode`s and the
  `fill_mode`/`constant` overload example.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="calendar.html">
    <span class="label">Next</span>
    <span class="title">calendar</span>
  </a>
</div>
