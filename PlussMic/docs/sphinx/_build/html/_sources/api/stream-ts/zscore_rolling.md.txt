<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 68bbd64d1ff4 -->

# StreamTimeSeriesAccessor::zscore_rolling

[Back to StreamTimeSeriesAccessor API Reference](../../stream_ts_accessor_api_reference)

## Signature

```cpp
Stream StreamTimeSeriesAccessor::zscore_rolling(
    std::int64_t window, std::int64_t min_periods = 1, double epsilon = 1e-12) const
```

## What It Does

Computes a **rolling z-score**: at each position, `(x - rolling_mean) /
rolling_std`, using a trailing window of `window` positions ending at (and
including) that position. The rolling mean and standard deviation use a
**population** convention (dividing by the window's observed count, not
count `- 1`) — there is no `ddof` parameter here. A position is `NULL` if
the rolling standard deviation isn't usefully positive
(`|std| <= epsilon`), isn't finite, or `x` itself isn't finite at that
position.

```{important} a single missing value poisons every window it touches
Unlike [`rolling()`](rolling.md)/[`expanding()`](expanding.md)/
[`grouped()`](grouped.md), this method's underlying engine does **not** skip
missing values within a window — it accumulates every position in the
window with plain floating-point arithmetic, so one missing entry makes the
mean *and* std (and therefore the z-score) come back `NULL` for every
position whose window still contains it. The effect clears on its own once
the missing position scrolls out of the trailing window.
```

```{important} StreamTimeSeriesAccessor requires the bound stream to be numeric Series-kind
Same precondition as every method on this accessor — see
[`Stream::ts`](../stream/ts.md)'s own important callout for the exact throw
message pattern.
```

## Parameters

**window** (`std::int64_t`)
    The trailing window size, in positions. Must be positive, or it throws.

**min_periods** (`std::int64_t`, default `1`)
    The minimum number of positions that must actually exist in a window
    near the start of the stream.

**epsilon** (`double`, default `1e-12`)
    The smallest rolling standard deviation still treated as usefully
    nonzero. Must be positive, or it throws.

## Returns

`Stream`
    A new stream, same length as the bound source, holding the rolling
    z-score at each position.

## Examples

Example 1. A rolling z-score over a clean (no missing values) stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = stream.ts().zscore_rolling(3);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=5
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | nan                                                                                                      
1            | 1                                                                                                        
2            | 1.22474487139                                                                                            
3            | 1.22474487139                                                                                            
4            | 1.22474487139                                                                                            
========================================================================================================================
```

Position `0`'s window is just itself — a single-value window always has a
standard deviation of exactly `0`, which is below `epsilon`, so it's `nan`.
Position `1`'s window is `(10, 20)`: mean `15`, population std `5`, so
`(20 - 15) / 5 = 1`.

## Notes

- The rolling mean/std here always use a **population** convention.
- A window whose standard deviation is `0` produces `NULL`, not a division
  by a near-zero number.
- See the important callouts above.
- `window` must be `> 0`, or it throws. `epsilon` must be `> 0`, or it
  throws `"subsp::stats::ts::zscore_rolling: epsilon must be > 0"`.

## See Also

- [`StreamTimeSeriesAccessor::rolling`](rolling.md): the fixed-window engine
  used for most other window statistics, which does skip missing values
  within a window rather than poisoning it.
- [`StreamTimeSeriesAccessor::rolling_block`](rolling_block.md): shares this
  method's underlying rolling-mean/rolling-std engine.
- [`Stream::ts`](../stream/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::zscore_rolling`](../slate-ts/zscore_rolling.md): the
  underlying single-slate engine, with the fully worked
  missing-value-poisons-the-window example.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timed.html">
    <span class="label">Previous</span>
    <span class="title">timed</span>
  </a>
</div>
