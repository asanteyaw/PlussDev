<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 68b18d9b8e42 -->

# AtlasTsResampler::bfill

[Back to AtlasTsResampler API Reference](../../atlas_ts_resampler_api_reference)

## Signature

```cpp
AtlasTsResampler::bfill() const -> Atlas
```

## What It Does

Reindexes each frame's original (unaggregated) rows onto that frame's own regular
grid produced by [`AtlasTsAccessor::resample()`](../atlas-ts/resample.md), using
backward-fill (`Forward`-looking) tick matching — each grid tick takes the value of
that frame's nearest source observation at or after it — the same transform
[`DataFrameTsResampler::bfill`](../dataframe-ts-resample/bfill.md) applies to a
single frame's resampler, broadcast across every frame the parent
[`resample()`](../atlas-ts/resample.md) call was bound to. A tick with no eligible
later observation, within that frame, produces `NULL`.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with one row per tick
    in each frame's regular grid, each taking that frame's next known value at or
    after that tick.

## Examples

Example 1. Backward-fill two frames' 3-point series onto their own daily grids.

```cpp
using namespace pluss::table;

auto q1 = DataFrame(
    {{"value", Thread(pluss::make_series({100.0, 102.0, 110.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
auto q2 = DataFrame(
    {{"value", Thread(pluss::make_series({200.0, 204.0, 220.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().resample("D").bfill();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | 110  
2024-01-04 | 110  
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 200  
2024-01-02 | 204  
2024-01-03 | 220  
2024-01-04 | 220  
2024-01-05 | 220  
------------------
[5 cs x 1 feat]
```

`frame(1)`'s `2024-01-03`/`2024-01-04` ticks take `frame(1)`'s own `2024-01-05`
value (`220`), never `frame(0)`'s `110`.

## Notes

- A grid tick later than every source observation, within that frame, has no next
  value to pull from and produces `NULL`.
- Each frame is reindexed independently, from that frame's own observations only.

## See Also

- [`DataFrameTsResampler::bfill`](../dataframe-ts-resample/bfill.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTsResampler::ffill`](ffill.md), [`AtlasTsResampler::asfreq`](asfreq.md),
  [`AtlasTsResampler::nearest`](nearest.md): the other reindex modes.
- [`AtlasTsAccessor::resample`](../atlas-ts/resample.md): the entry point that
  produces this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asfreq.html">
    <span class="label">Previous</span>
    <span class="title">asfreq</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
