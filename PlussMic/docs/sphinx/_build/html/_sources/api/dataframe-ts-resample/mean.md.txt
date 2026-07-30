<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dc609e9ebb3d -->

# DataFrameTsResampler::mean

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrameTsResampler::mean() const -> DataFrame
```

## What It Does

Aggregates every bucket built by [`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md)
with the mean of its observations. Equivalent to
`accessor.resample(freq/options, ResampleOp::Mean)`. A bucket with zero
observations produces `NULL`.

## Returns

`DataFrame`
    One row per bucket in the regular grid, each feature replaced by its bucket
    mean.

## Examples

Example 1. Daily mean, including an empty bucket.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").mean();
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
2024-01-01 | 15   
2024-01-02 | NULL 
2024-01-03 | 17.5 
------------------
[3 cs x 1 feat]
```

`2024-01-01`'s bucket averages `(10, 20)` to `15`; `2024-01-02` has no
observations at all, so it's `NULL` (not `0` — unlike [`sum()`](sum.md)); 
`2024-01-03` averages `(30, 5)` to `17.5`.

## Notes

- Every bucket in the contiguous grid between the data's minimum and maximum tick
  appears, even ones with zero observations.
- `mean()` on an empty bucket is `NULL`; compare with [`sum()`](sum.md) and
  [`count()`](count.md), which produce `0` for the same empty bucket instead.

## See Also

- [`DataFrameTsResampler::sum`](sum.md), [`DataFrameTsResampler::min`](min.md),
  [`DataFrameTsResampler::max`](max.md),
  [`DataFrameTsResampler::first`](first.md), [`DataFrameTsResampler::last`](last.md),
  [`DataFrameTsResampler::count`](count.md): the other bucket aggregates.
- [`DataFrameTsAccessor::resample`](../dataframe-ts/resample.md): the entry point
  that produces this accessor, and the full frequency-string grammar.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
