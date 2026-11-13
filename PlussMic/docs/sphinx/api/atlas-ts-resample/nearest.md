<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 18be2ae08c60 -->

# AtlasTsResampler::nearest

[Back to AtlasTsResampler API Reference](../../atlas_ts_resampler_api_reference)

## Signature

```cpp
Atlas AtlasTsResampler::nearest() const
```

## What It Does

Reindexes each frame's original (unaggregated) rows onto that frame's own regular
grid produced by [`AtlasTsAccessor::resample()`](../atlas-ts/resample.md), using
nearest-tick matching — each grid tick takes the value of whichever of that frame's
source observations is closest to it in time (ties break toward the earlier
candidate) — the same transform
[`DataFrameTsResampler::nearest`](../dataframe-ts-resample/nearest.md) applies to a
single frame's resampler, broadcast across every frame the parent
[`resample()`](../atlas-ts/resample.md) call was bound to. A tick with no source
observations at all, within that frame, produces `NULL`.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with one row per tick
    in each frame's regular grid, each taking that frame's closest source
    observation's value.

## Examples

Example 1. Nearest-fill two frames' 3-point series onto their own daily grids.

```cpp
using namespace pluss::table;

auto q1 = DataFrame(
    {{"value", Slate(pluss::make_series({100.0, 102.0, 110.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
auto q2 = DataFrame(
    {{"value", Slate(pluss::make_series({200.0, 204.0, 220.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().resample("D").nearest();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | 102  
2024-01-04 | 110  
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 200  
2024-01-02 | 204  
2024-01-03 | 204  
2024-01-04 | 220  
2024-01-05 | 220  
------------------
[5 cs x 1 feat]
```

`frame(1)`'s `2024-01-03` is 1 day from its own `2024-01-02` (`204`) and 2 days
from its own `2024-01-05` (`220`), so it takes `204` — the same tie-break logic as
`frame(0)`, computed independently.

## Notes

- Compare with [`ffill()`](ffill.md) (always the prior observation) and
  [`bfill()`](bfill.md) (always the next observation) — `nearest()` picks whichever
  of the two is closer, independently for every grid tick, within each frame.
- Each frame is reindexed independently, from that frame's own observations only.

## See Also

- [`DataFrameTsResampler::nearest`](../dataframe-ts-resample/nearest.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTsResampler::ffill`](ffill.md), [`AtlasTsResampler::bfill`](bfill.md),
  [`AtlasTsResampler::asfreq`](asfreq.md): the other reindex modes.
- [`AtlasTsAccessor::resample`](../atlas-ts/resample.md): the entry point that
  produces this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
