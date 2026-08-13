<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4b9b255049de -->

# StreamTimeSeriesAccessor::lead

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::lead(std::int64_t periods = 1) const
```

## What It Does

Shifts the bound stream `periods` positions backward:
`out[i] = x[i + periods]` where that index is in bounds, `NULL` otherwise. A
negative `periods` delegates to [`lag()`](lag.md) with the sign flipped.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of positions to shift backward. Negative values behave as
    [`lag(-periods)`](lag.md) instead.

## Returns

`Stream`
    A new stream, same length as the bound source, led by `periods`
    positions.

## Examples

Example 1. Lead a numeric stream by one position.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = stream.ts().lead(1);
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
0            | 20                                                                                                       
1            | 15                                                                                                       
2            | 30                                                                                                       
3            | nan                                                                                                      
========================================================================================================================
```

Each position now shows the next position's value; position `3` has no
successor, so it's `nan` — the `nan` lands at the opposite end from
[`lag()`](lag.md).

## Notes

- `lead(periods)` with a negative `periods` is exactly `lag(-periods)`.
- A pre-existing missing entry in the source stream is honored, not just the
  `NULL`s the shift itself introduces.
- See the important callout above.

## See Also

- [`StreamTimeSeriesAccessor::lag`](lag.md): the mirror-image shift.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`CatalogTimeSeriesAccessor::lead`](../catalog-ts/lead.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag_block.html">
    <span class="label">Previous</span>
    <span class="title">lag_block</span>
  </a>
  <a class="next-link" href="log_returns.html">
    <span class="label">Next</span>
    <span class="title">log_returns</span>
  </a>
</div>
