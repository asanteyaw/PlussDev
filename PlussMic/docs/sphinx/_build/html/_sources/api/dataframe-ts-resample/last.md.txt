<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dcdafb42d5a5 -->

# DataFrameTsResampler::last

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrameTsResampler::last() const -> DataFrame
```

## What It Does

Aggregates every bucket built by [`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md)
with its latest observation (in original tick order). Equivalent to
`accessor.resample(freq/options, ResampleOp::Last)`. A bucket with zero
observations produces `NULL`.

## Returns

`DataFrame`
    One row per bucket in the regular grid, each feature replaced by the value
    of its bucket's latest observation.

## Examples

Example 1. Daily last-value, including an empty bucket.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Slate(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").last();
std::cout << out;
```

### Output (before)

```text
===========================
                    | value
---------------------------
2024-01-01 08:00:00 | 10   
2024-01-01 20:00:00 | 20   
2024-01-03 10:00:00 | 30   
2024-01-03 15:00:00 | 5    
---------------------------
[4 cs x 1 feat]
```

### Output (after)

```text
==================
           | value
------------------
2024-01-01 | 20   
2024-01-02 | NULL 
2024-01-03 | 5    
------------------
[3 cs x 1 feat]
```

`2024-01-01`'s latest observation is `20` (at `20:00`, after `10`'s `08:00`);
`2024-01-02` has no observations, so it's `NULL`; `2024-01-03`'s latest is `5`
(at `15:00`, after `30`'s `10:00`).

## Notes

- Every bucket in the contiguous grid between the data's minimum and maximum tick
  appears, even ones with zero observations.
- `last()` on an empty bucket is `NULL`; compare with [`sum()`](sum.md) and
  [`count()`](count.md), which produce `0` for the same empty bucket instead.

## See Also

- [`DataFrameTsResampler::first`](first.md), [`DataFrameTsResampler::min`](min.md),
  [`DataFrameTsResampler::max`](max.md), [`DataFrameTsResampler::mean`](mean.md),
  [`DataFrameTsResampler::sum`](sum.md), [`DataFrameTsResampler::count`](count.md):
  the other bucket aggregates.
- [`DataFrameTsAccessor::resample`](../dataframe-ts/resample.md): the entry point
  that produces this accessor, and the full frequency-string grammar.

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
