<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 330c5e554e05 -->

# StreamTimeSeriesAccessor::fill_gaps

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::fill_gaps(
    pluss::subsp::stats::ts::GapFillMode mode = pluss::subsp::stats::ts::GapFillMode::Forward,
    std::optional<double> constant = std::nullopt) const
```

## What It Does

Fills missing entries in the bound stream using one of four strategies:
`Forward` (the default, nearest **prior** real value), `Backward` (nearest
**following** real value), `Zero` (every missing position becomes `0`), or
`Constant` (every missing position becomes `constant`, required for this
mode). A gap with no real value on the required side is left missing.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**mode** (`pluss::subsp::stats::ts::GapFillMode`, default `Forward`)
    Which fill strategy to use.

**constant** (`std::optional<double>`, default `std::nullopt`)
    The fill value for `Constant`. Required (throws if absent) when `mode`
    is `Constant`.

## Returns

`Stream`
    A new stream, same length as the bound source, with gaps filled per
    `mode`.

## Examples

Example 1. `Forward` (the default) — each gap takes the nearest prior value.

```cpp
auto with_gap = pluss::kindle::Stream::from_slate(pluss::table::Slate(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})));
auto out = with_gap.ts().fill_gaps();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=4
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 10                                                                                                       
2            | 30                                                                                                       
3            | 40                                                                                                       
========================================================================================================================
```

Position `1` (masked-missing) takes position `0`'s `10` — the nearest prior
real value. Positions `2` and `3` were never missing, so they're untouched.

## Notes

- A gap with nothing to fill from on the required side (a leading gap under
  `Forward`, a trailing gap under `Backward`) is left `NULL` rather than
  inventing a value.
- `GapFillMode::Constant` throws
  `"subsp::stats::ts::fill_gaps: constant mode requires a value"` if
  `constant` is absent.
- See the important callout above.

## See Also

- [`Stream::ffill`](../stream/ffill.md), [`Stream::bfill`](../stream/bfill.md):
  the equivalent top-level operations outside the `ts()` accessor.
- [`StreamTimeSeriesAccessor::align_to`](align_to.md): aligning onto a
  different time axis, optionally filling any newly-introduced gaps the
  same way.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::fill_gaps`](../slate-ts/fill_gaps.md): the
  underlying single-slate engine, with additional edge-case examples.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="expanding.html">
    <span class="label">Previous</span>
    <span class="title">expanding</span>
  </a>
  <a class="next-link" href="grouped.html">
    <span class="label">Next</span>
    <span class="title">grouped</span>
  </a>
</div>
