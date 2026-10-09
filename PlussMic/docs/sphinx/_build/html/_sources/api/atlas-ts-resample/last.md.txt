<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ad27aca49d51 -->

# AtlasTsResampler::last

[Back to AtlasTsResampler API Reference](../../atlas_ts_resampler_api_reference)

## Signature

```cpp
AtlasTsResampler::last() const -> Atlas
```

## What It Does

Aggregates every bucket built by [`AtlasTsAccessor::resample()`](../atlas-ts/resample.md),
in **every frame of the stack**, with its latest (last-in-time) observation — the
same transform [`DataFrameTsResampler::last`](../dataframe-ts-resample/last.md)
applies to a single frame's resampler, broadcast across every frame the parent
[`resample()`](../atlas-ts/resample.md) call was bound to. A bucket with zero
observations, within a given frame, produces `NULL`.

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with one row per bucket
    in each frame's regular grid, each feature replaced by its bucket's latest
    observation.

## Examples

Example 1. Daily last-observation across two frames sharing the same gapped date
range.

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

auto out = atlas.ts().resample("D").last();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 20   
2024-01-02 | nan  
2024-01-03 | 5    
------------------
[3 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 2    
2024-01-02 | nan  
2024-01-03 | 4    
------------------
[3 cs x 1 feat]
```

Each frame's `2024-01-01` bucket keeps its own `20:00` reading (the latest of the
two); `2024-01-03` keeps its own `15:00` reading.

## Notes

- Every bucket in each frame's own contiguous grid appears, even ones with zero
  observations, coming back `NULL`.
- Each frame's buckets are aggregated independently, from that frame's own
  observations only.

## See Also

- [`DataFrameTsResampler::last`](../dataframe-ts-resample/last.md): the
  single-frame operation this method broadcasts across the stack.
- [`AtlasTsResampler::first`](first.md): the mirror-image aggregate.
- [`AtlasTsAccessor::resample`](../atlas-ts/resample.md): the entry point that
  produces this accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="first.html">
    <span class="label">Previous</span>
    <span class="title">first</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
