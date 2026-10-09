<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7308505a9b38 -->

# Stream::ts

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
StreamTimeSeriesAccessor Stream::ts() const
```

## What It Does

The entry point into time-series transformations on the stream. `ts()` binds
the current stream and hands back a `StreamTimeSeriesAccessor`, which exposes 17
methods: period-shift transforms (`returns`, `log_returns`, `diff`, `lag`,
`lead`), windowed statistics (`rolling` — three overloads, `timed`, `grouped`,
`expanding`, `ewm`, `zscore_rolling`), gap/alignment utilities (`fill_gaps`,
`align_to`, `sort`), block extraction (`lag_block`, `rolling_block`), and
calendar decomposition (`calendar`) — see the
[StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)
for every method's own dedicated page.

```{important} A much larger surface than CatalogTimeSeriesAccessor
`Stream::ts()` exposes 17 methods — `Catalog::ts()` exposes only 6 (the
period-shift family plus one basic `rolling()` overload). The extra 11 methods
here (`expanding`, `ewm`, `zscore_rolling`, `fill_gaps`, `align_to`, `lag_block`,
`rolling_block`, `calendar`, `sort`, and the axis-driven `rolling()`/`timed()`/
`grouped()` overloads) need a companion time axis or per-stream context that a
multi-key catalog can't supply implicitly — see
[`Catalog::ts`](../catalog/ts.md)'s own important callout for the reverse view of
this same asymmetry.
```

```{important} Requires the stream itself to be numeric Series-kind
Calling any method on this accessor with a non-numeric stream (`Note`,
`Timeline`, `TimeIndex`) throws — the exact message names whichever method
actually ran, e.g. `"TimeSeriesAccessor::diff: source must be Series"`. Cast
first with [`Stream::cast(SlateCastTarget::Series)`](cast.md) if the stream
isn't already numeric.
```

## Parameters

`None`

## Returns

`StreamTimeSeriesAccessor`
    A binder for the current stream — call one of its 17 methods to shift,
    window-aggregate, or otherwise transform the stream along a time dimension.

## Examples

Example 1. `ts().diff(1)` computes the one-period absolute change — see
[`StreamTimeSeriesAccessor::diff`](../stream-ts/diff.md) for this specific
method's own page.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0}));
auto out = stream.ts().diff(1);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | nan                                                                                                      
1            | 10                                                                                                       
2            | -5                                                                                                       
========================================================================================================================
```

Example 2 (edge case). A non-numeric stream throws — see the second important
callout above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"a", "b"}));
try {
  auto out = stream.ts().diff(1);
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
TimeSeriesAccessor::diff: source must be Series
```

## Notes

- Not `noexcept`; the returned accessor itself does no work, but every method it
  exposes eagerly re-checks the bound stream's kind.
- See the important callouts above — both the surface-size asymmetry with
  `Catalog::ts()` and the numeric-only precondition apply.
- Requires a defined stream; throws `"Stream: undefined handle"` otherwise.

## See Also

- [StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference):
  the full list of 17 methods this accessor exposes.
- [`Stream::nt`](nt.md) / [`Stream::tl`](tl.md): the sibling accessors for
  text-processing and timestamp-feature operations.
- [`Stream::cast`](cast.md): convert a non-numeric stream to `Series` first.
- [`Catalog::ts`](../catalog/ts.md): the whole-catalog equivalent, with a much
  smaller method surface — see the important callout above.
- [`TimeSeriesAccessor` (Slate)](../slate-ts/returns.md): the underlying
  single-column engine this accessor's methods delegate to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_slate.html">
    <span class="label">Previous</span>
    <span class="title">to_slate</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
