<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 72a75eb7a1fc -->

# DataFrameTsResampler::sum

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrameTsResampler::sum() const -> DataFrame
```

## What It Does

Aggregates every bucket built by [`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md)
with the sum of its observations. Equivalent to
`accessor.resample(freq/options, ResampleOp::Sum)`. Unlike [`mean()`](mean.md),
[`min()`](min.md), [`max()`](max.md), [`first()`](first.md), and
[`last()`](last.md), a bucket with zero observations produces `0`, not `NULL`.

## Returns

`DataFrame`
    One row per bucket in the regular grid, each feature replaced by its bucket
    sum.

## Examples

Example 1. Daily sum, including an empty bucket.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Slate(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").sum();
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
2024-01-01 | 30   
2024-01-02 | 0    
2024-01-03 | 35   
------------------
[3 cs x 1 feat]
```

`2024-01-01`'s bucket sums `(10, 20)` to `30`; `2024-01-02` has no observations
at all, so it's `0` (not `NULL` — unlike [`mean()`](mean.md)); `2024-01-03`
sums `(30, 5)` to `35`.

## Notes

- Every bucket in the contiguous grid between the data's minimum and maximum tick
  appears, even ones with zero observations.
- `sum()` on an empty bucket is `0`; compare with [`count()`](count.md), which
  is also `0` for an empty bucket, versus [`mean()`](mean.md)/[`min()`](min.md)/
  [`max()`](max.md)/[`first()`](first.md)/[`last()`](last.md), which are `NULL`.

## See Also

- [`DataFrameTsResampler::mean`](mean.md), [`DataFrameTsResampler::min`](min.md),
  [`DataFrameTsResampler::max`](max.md),
  [`DataFrameTsResampler::first`](first.md), [`DataFrameTsResampler::last`](last.md),
  [`DataFrameTsResampler::count`](count.md): the other bucket aggregates.
- [`DataFrameTsAccessor::resample`](../dataframe-ts/resample.md): the entry point
  that produces this accessor, and the full frequency-string grammar.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nearest.html">
    <span class="label">Previous</span>
    <span class="title">nearest</span>
  </a>
</div>
