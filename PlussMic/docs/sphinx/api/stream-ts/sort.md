<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ba23361a131 -->

# StreamTimeSeriesAccessor::sort

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::sort(const Stream& axis, bool ascending = true, bool nulls_last = true) const
```

## What It Does

Reorders the bound stream's values into chronological order along `axis`.
The result carries `axis`, now sorted, as its own case labels.

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**axis** (`const Stream&`)
    A `Timeline`/`TimeIndex` stream, the same length as the bound stream, to
    sort by.

**ascending** (`bool`, default `true`)
    Sort chronologically forward (`true`) or backward (`false`).

**nulls_last** (`bool`, default `true`)
    Whether undefined axis positions sort after (`true`) or before (`false`)
    every defined position.

## Returns

`Stream`
    A new stream containing the bound values, reordered into chronological
    order along `axis`, with `axis` as its case labels.

## Examples

Example 1. Sort a stream by an out-of-order `Timeline` axis.

```cpp
auto axis = pluss::kindle::Stream(pluss::make_timeline({
    "2024-01-03T00:00:00Z", "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"}));
auto values = pluss::kindle::Stream(pluss::make_series({30.0, 10.0, 20.0}));
auto out = values.ts().sort(axis);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
2024-01-01T~ | 10                                                                                                       
2024-01-02T~ | 20                                                                                                       
2024-01-03T~ | 30                                                                                                       
========================================================================================================================
```

`axis`'s ticks and `values`' entries move together — `10` (originally paired
with `2024-01-01`) now leads the result.

## Notes

- `axis` must be the same length as the bound stream.
- See the important callout above.

## See Also

- [`StreamTimeSeriesAccessor::rolling`](rolling.md) (calendar-duration
  overload), [`timed()`](timed.md): other methods that rely on the same
  time-axis idea.
- [`Stream::lbl`](../stream/lbl.md): the general case-label accessor,
  including its own sort-by-label surface.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`DataFrameTsAccessor::sort`](../dataframe-ts/sort.md): the DataFrame-level
  equivalent, with an explicit no-explicit-axis throw example.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_block.html">
    <span class="label">Previous</span>
    <span class="title">rolling_block</span>
  </a>
  <a class="next-link" href="timed.html">
    <span class="label">Next</span>
    <span class="title">timed</span>
  </a>
</div>
