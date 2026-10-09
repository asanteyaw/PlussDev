<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8bbf972cb588 -->

# AtlasTsResampler::ffill

[Back to AtlasTsResampler API Reference](../../atlas_ts_resampler_api_reference)

## Signature

```cpp
AtlasTsResampler::ffill() const -> Atlas
```

## What It Does

Reindexes each frame's original (unaggregated) rows onto that frame's own regular
grid produced by [`AtlasTsAccessor::resample()`](../atlas-ts/resample.md), using
forward-fill (`Backward`-looking) tick matching — each grid tick takes the value of
that frame's most recent source observation at or before it — the same transform
[`DataFrameTsResampler::ffill`](../dataframe-ts-resample/ffill.md) applies to a
single frame's resampler, broadcast across every frame the parent
[`resample()`](../atlas-ts/resample.md) call was bound to. A tick with no eligible
prior observation, within that frame, produces `NULL`.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with one row per tick
    in each frame's regular grid, each carrying forward that frame's last known
    value at or before that tick.

## Examples

Example 1. Forward-fill two frames' 3-point series onto their own daily grids.

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

auto out = atlas.ts().resample("D").ffill();
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
2024-01-04 | 102  
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 200  
2024-01-02 | 204  
2024-01-03 | 204  
2024-01-04 | 204  
2024-01-05 | 220  
------------------
[5 cs x 1 feat]
```

`frame(1)`'s `2024-01-03`/`2024-01-04` ticks carry forward `frame(1)`'s own
`2024-01-02` value (`204`), never `frame(0)`'s `102`.

## Notes

- A grid tick earlier than every source observation, within that frame, has no
  prior value to carry forward and produces `NULL`.
- Each frame is reindexed independently, from that frame's own observations only.

## See Also

- [`DataFrameTsResampler::ffill`](../dataframe-ts-resample/ffill.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTsResampler::bfill`](bfill.md), [`AtlasTsResampler::asfreq`](asfreq.md),
  [`AtlasTsResampler::nearest`](nearest.md): the other reindex modes.
- [`AtlasTsAccessor::resample`](../atlas-ts/resample.md): the entry point that
  produces this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="first.html">
    <span class="label">Next</span>
    <span class="title">first</span>
  </a>
</div>
