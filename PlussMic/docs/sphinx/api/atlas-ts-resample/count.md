<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 95f4fbac2e12 -->

# AtlasTsResampler::count

[Back to AtlasTsResampler API Reference](../../atlas_ts_resampler_api_reference)

## Signature

```cpp
Atlas AtlasTsResampler::count() const
```

## What It Does

Aggregates every bucket built by [`AtlasTsAccessor::resample()`](../atlas-ts/resample.md),
in **every frame of the stack**, with the number of observations it holds — the
same transform [`DataFrameTsResampler::count`](../dataframe-ts-resample/count.md)
applies to a single frame's resampler, broadcast across every frame the parent
[`resample()`](../atlas-ts/resample.md) call was bound to. A bucket with zero
observations, within a given frame, produces `0`.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with one row per bucket
    in each frame's regular grid, each feature replaced by its bucket's observation
    count.

## Examples

Example 1. Daily count across two frames sharing the same gapped date range.

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

auto out = atlas.ts().resample("D").count();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 2    
2024-01-02 | 0    
2024-01-03 | 2    
------------------
[3 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 2    
2024-01-02 | 0    
2024-01-03 | 2    
------------------
[3 cs x 1 feat]
```

Both frames happen to share the same row counts here (2 observations per non-empty
bucket) since they share the same timestamps — but each frame's count is computed
purely from its own observations, never the other frame's.

## Notes

- Every bucket in each frame's own contiguous grid appears, even ones with zero
  observations.
- Like [`sum()`](sum.md), `count()` on an empty bucket is `0`, not `NULL`.
- Each frame's buckets are aggregated independently, from that frame's own
  observations only.

## See Also

- [`DataFrameTsResampler::count`](../dataframe-ts-resample/count.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTsResampler::sum`](sum.md), [`AtlasTsResampler::mean`](mean.md),
  [`AtlasTsResampler::min`](min.md), [`AtlasTsResampler::max`](max.md),
  [`AtlasTsResampler::first`](first.md), [`AtlasTsResampler::last`](last.md): the
  other bucket aggregates.
- [`AtlasTsAccessor::resample`](../atlas-ts/resample.md): the entry point that
  produces this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
