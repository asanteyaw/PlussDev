<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8851632fe714 -->

# AtlasTsResampler::sum

[Back to AtlasTsResampler API Reference](../../atlas_ts_resampler_api_reference)

## Signature

```cpp
Atlas AtlasTsResampler::sum() const
```

## What It Does

Aggregates every bucket built by [`AtlasTsAccessor::resample()`](../atlas-ts/resample.md),
in **every frame of the stack**, with the sum of its observations — the same
transform [`DataFrameTsResampler::sum`](../dataframe-ts-resample/sum.md) applies to
a single frame's resampler, broadcast across every frame the parent
[`resample()`](../atlas-ts/resample.md) call was bound to. A bucket with zero
observations, within a given frame, produces `0`.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with one row per bucket
    in each frame's regular grid, each feature replaced by its bucket sum.

## Examples

Example 1. Daily sum across two frames sharing the same gapped date range.

```cpp
using namespace pluss::table;

auto q1 = DataFrame(
    {{"value", Slate(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
auto q2 = DataFrame(
    {{"value", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().resample("D").sum();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 30   
2024-01-02 | 0    
2024-01-03 | 35   
------------------
[3 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 3    
2024-01-02 | 0    
2024-01-03 | 7    
------------------
[3 cs x 1 feat]
```

Each frame's `2024-01-02` bucket (zero observations, within that frame) is `0`, not
`NULL` — unlike [`mean()`](mean.md).

## Notes

- Every bucket in each frame's own contiguous grid appears, even ones with zero
  observations.
- Like [`count()`](count.md), `sum()` on an empty bucket is `0`, not `NULL`.
- Each frame's buckets are aggregated independently, from that frame's own
  observations only.

## See Also

- [`DataFrameTsResampler::sum`](../dataframe-ts-resample/sum.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTsResampler::mean`](mean.md), [`AtlasTsResampler::count`](count.md): the
  other bucket aggregates that share `sum()`'s or differ from its empty-bucket
  convention.
- [`AtlasTsAccessor::resample`](../atlas-ts/resample.md): the entry point that
  produces this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nearest.html">
    <span class="label">Previous</span>
    <span class="title">nearest</span>
  </a>
</div>
