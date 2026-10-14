<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a3069fa432c8 -->

# StreamTimeSeriesAccessor::lag_block

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
pluss::table::DataFrame StreamTimeSeriesAccessor::lag_block(pluss::msharp::RangeView::Int lags) const
```

## What It Does

Computes several [`lag()`](lag.md) shifts of the bound stream at once, each
as its own output column named `"lag" + <lag value>`. Requires at least one
lag.

```{important} Returns a DataFrame, not a Stream
Unlike every other method on this accessor except [`rolling_block()`](rolling_block.md)
and [`calendar()`](calendar.md), `lag_block()` leaves the kindle model
entirely — its result is a `pluss::table::DataFrame`, not a `Stream`.
```

## Parameters

**lags** (`pluss::msharp::RangeView::Int`)
    The lag distances to compute, in the order given. Must not be empty, or
    it throws.

## Returns

`pluss::table::DataFrame`
    A new frame with one column per requested lag (named `"lag1"`, `"lag2"`,
    and so on).

## Examples

Example 1. Compute two lags of the same stream at once.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = stream.ts().lag_block({1, 2});
std::cout << out;
```

### Output

```text
=================
     | lag1  lag2
-----------------
0    | nan   nan 
1    | 10    nan 
2    | 20    10  
3    | 15    20  
-----------------
[4 cs x 2 feat]
```

`lag1` is the same as [`lag(1)`](lag.md). `lag2` is [`lag(2)`](lag.md): each
position now shows the value from two positions earlier.

Example 2 (edge case). An empty `lags` throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
try {
  auto out = stream.ts().lag_block({});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
TimeSeriesAccessor::lag_block: expected at least one lag
```

## Notes

- See the important callout above.
- Output columns are always named `"lag" + <value>` — there's no way to
  customize the column names from this method directly.
- A pre-existing missing entry in the source stream is honored in every lag
  column, the same as [`lag()`](lag.md) itself.

## See Also

- [`StreamTimeSeriesAccessor::lag`](lag.md): the single-lag building block
  this method computes several of at once.
- [`StreamTimeSeriesAccessor::rolling_block`](rolling_block.md): the
  equivalent "several at once" pattern for rolling window statistics.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::lag_block`](../slate-ts/lag_block.md): the
  underlying single-slate engine.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag.html">
    <span class="label">Previous</span>
    <span class="title">lag</span>
  </a>
  <a class="next-link" href="lead.html">
    <span class="label">Next</span>
    <span class="title">lead</span>
  </a>
</div>
