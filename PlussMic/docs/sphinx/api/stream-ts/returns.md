<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3f97d5a7d7b8 -->

# StreamTimeSeriesAccessor::returns

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::returns(std::int64_t periods = 1) const
```

## What It Does

Computes the fractional period-over-period change of the bound stream:
`out[i] = x[i] / base[i] - 1`, where `base` is `x` shifted by `periods` (a lag
when `periods > 0`, a lead when `periods < 0`). A position is `NULL` wherever
`x[i]` or `base[i]` is missing, `base[i]` is `0`, or either value is
non-finite.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Every method on this accessor routes through `Slate::ts()` for the bound
stream. If the stream isn't a numeric `Series` (`Note`, `Timeline`,
`TimeIndex`, or a non-numeric `Series`), the call throws — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Positive shifts
    compare each position against an earlier one (a lag); negative shifts
    compare against a later one (a lead). Must not be `0`, or the call
    throws.

## Returns

`Stream`
    A new stream, same length as the bound source, holding the
    period-over-period fractional return.

## Examples

Example 1. Compute the one-period return of a numeric stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = stream.ts().returns(1);
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
0            | nan                                                                                                      
1            | 1                                                                                                        
2            | -0.25                                                                                                    
3            | 1                                                                                                        
========================================================================================================================
```

Position `0` has no prior position to compare against, so it's `nan`.
Position `1` is `20 / 10 - 1 = 1` (a 100% increase); position `2` is
`15 / 20 - 1 = -0.25` (a 25% decrease); position `3` is `30 / 15 - 1 = 1`.

Example 2 (edge case). `periods = 0` throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
try {
  auto out = stream.ts().returns(0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
subsp::stats::ts::returns: periods cannot be zero
```

## Notes

- A `base` value of exactly `0` always produces `NULL`, regardless of `x[i]`,
  since the fractional change would otherwise be undefined or infinite.
- A pre-existing missing entry in the source stream is honored — it produces
  `NULL` in every return that would otherwise reference it.
- `periods` must not be `0`, or it throws
  `"subsp::stats::ts::returns: periods cannot be zero"`.
- See the important callout above.

## See Also

- [`StreamTimeSeriesAccessor::log_returns`](log_returns.md): the logarithmic
  counterpart, additionally requiring both compared values to be strictly
  positive.
- [`StreamTimeSeriesAccessor::diff`](diff.md): the absolute (non-fractional)
  period-over-period change.
- [`StreamTimeSeriesAccessor::lag`](lag.md), [`lead()`](lead.md): the
  underlying shift this method's comparison base is built from.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`CatalogTimeSeriesAccessor::returns`](../catalog-ts/returns.md): the
  whole-catalog equivalent.
- [`TimeSeriesAccessor::returns`](../slate-ts/returns.md): the underlying
  single-slate engine, with additional edge-case examples.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log_returns.html">
    <span class="label">Previous</span>
    <span class="title">log_returns</span>
  </a>
  <a class="next-link" href="rolling.html">
    <span class="label">Next</span>
    <span class="title">rolling</span>
  </a>
</div>
