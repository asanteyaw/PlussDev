<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 71c74430db31 -->

# StreamTimeSeriesAccessor::log_returns

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::log_returns(std::int64_t periods = 1) const
```

## What It Does

Computes the logarithmic period-over-period change of the bound stream:
`out[i] = ln(x[i] / base[i])`, where `base` is `x` shifted by `periods` (a lag
when `periods > 0`, a lead when `periods < 0`). A position is `NULL` wherever
`x[i]` or `base[i]` is missing or non-finite, **or either value is not
strictly positive** — unlike [`returns()`](returns.md), a zero or negative
value anywhere in the comparison always yields `NULL`.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base. Must not be `0`,
    or the call throws.

## Returns

`Stream`
    A new stream, same length as the bound source, holding the
    period-over-period log return.

## Examples

Example 1. Compute the one-period log return of a numeric stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = stream.ts().log_returns(1);
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
1            | 0.6931471806                                                                                             
2            | -0.2876820725                                                                                            
3            | 0.6931471806                                                                                             
========================================================================================================================
```

Position `1` is `ln(20 / 10) = ln(2) ≈ 0.6931`; position `2` is
`ln(15 / 20) ≈ -0.2877`; position `3` is `ln(30 / 15) ≈ 0.6931` — the same log
return as position `1`, since both moves double the value, whereas
[`returns()`](returns.md) would report the same `1.0` fractional return for
both but a different one for the halving case.

## Notes

- Unlike [`returns()`](returns.md), `log_returns()` requires **both** `x[i]`
  and `base[i]` to be strictly greater than `0`; a zero or negative value in
  either produces `NULL` rather than an error.
- Log returns are additive across periods, which plain fractional
  [`returns()`](returns.md) are not.
- `periods` must not be `0`, or it throws
  `"subsp::stats::ts::log_returns: periods cannot be zero"`.
- See the important callout above.

## See Also

- [`StreamTimeSeriesAccessor::returns`](returns.md): the fractional
  (non-logarithmic) counterpart, which tolerates zero or negative values more
  permissively.
- [`StreamTimeSeriesAccessor::diff`](diff.md): the absolute
  period-over-period change.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`CatalogTimeSeriesAccessor::log_returns`](../catalog-ts/log_returns.md):
  the whole-catalog equivalent.
- [`TimeSeriesAccessor::log_returns`](../thread-ts/log_returns.md): the
  underlying single-thread engine, with additional edge-case examples.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lead.html">
    <span class="label">Previous</span>
    <span class="title">lead</span>
  </a>
  <a class="next-link" href="returns.html">
    <span class="label">Next</span>
    <span class="title">returns</span>
  </a>
</div>
