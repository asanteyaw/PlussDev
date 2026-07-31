<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f85908f71e5a -->

# AtlasTsResampler::asfreq

[Back to AtlasTsResampler API Reference](../../atlas_ts_resampler_api_reference)

## Signature

```cpp
AtlasTsResampler::asfreq(std::optional<double> fill_value = std::nullopt) const -> Atlas
```

## What It Does

Reindexes each frame's original (unaggregated) rows onto that frame's own regular
grid produced by [`AtlasTsAccessor::resample()`](../atlas-ts/resample.md), using
exact tick matching — a grid tick keeps its source value only if that frame has a
source observation landing on that exact tick; otherwise it's `NULL` — the same
transform [`DataFrameTsResampler::asfreq`](../dataframe-ts-resample/asfreq.md)
applies to a single frame's resampler, broadcast across every frame the parent
[`resample()`](../atlas-ts/resample.md) call was bound to. This is unlike
[`mean()`](mean.md)/[`sum()`](sum.md)/etc., which aggregate every observation
*within* each bucket.

If `fill_value` is supplied, every grid tick (in every frame) that did **not** get
an exact match is replaced by that single constant instead of `NULL`.

## Parameters

**fill_value** (`std::optional<double>`, default `std::nullopt`)
    A constant substituted for every non-exact-match grid tick, in every frame.
    Leave as `std::nullopt` to keep those ticks `NULL`.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with one row per tick
    in each frame's regular grid, only exact matches (or the supplied `fill_value`)
    populated.

## Examples

Example 1. Reindex two frames' 3-point series onto their own daily grids.

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

auto out = atlas.ts().resample("D").asfreq();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | NULL 
2024-01-04 | NULL 
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 200  
2024-01-02 | 204  
2024-01-03 | NULL 
2024-01-04 | NULL 
2024-01-05 | 220  
------------------
[5 cs x 1 feat]
```

Each frame's `2024-01-03`/`2024-01-04` ticks have no source observation, within
that frame, landing exactly on them — so they're `NULL`, independently in both
frames.

## Notes

- `asfreq()` is a thin wrapper over `align_to` with exact tick matching — see
  [`DataFrameTsResampler::asfreq`](../dataframe-ts-resample/asfreq.md) for the full
  tick-matching contract.
- Unlike [`ffill()`](ffill.md)/[`bfill()`](bfill.md)/[`nearest()`](nearest.md),
  `asfreq()` never borrows a neighboring observation's value.
- Each frame is reindexed independently, from that frame's own observations only.

## See Also

- [`DataFrameTsResampler::asfreq`](../dataframe-ts-resample/asfreq.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTsResampler::ffill`](ffill.md), [`AtlasTsResampler::bfill`](bfill.md),
  [`AtlasTsResampler::nearest`](nearest.md): the other reindex modes.
- [`AtlasTsAccessor::resample`](../atlas-ts/resample.md): the entry point that
  produces this accessor.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
