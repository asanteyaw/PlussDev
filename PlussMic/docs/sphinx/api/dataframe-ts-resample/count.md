<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eade8285db21 -->

# DataFrameTsResampler::count

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrameTsResampler::count() const -> DataFrame
```

## What It Does

Aggregates every bucket built by [`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md)
with the number of observations it holds. Equivalent to
`accessor.resample(freq/options, ResampleOp::Count)`. A bucket with zero
observations produces `0`.

## Returns

`DataFrame`
    One row per bucket in the regular grid, each feature replaced by its bucket's
    observation count.

## Examples

Example 1. Daily count, including an empty bucket.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").count();
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
2024-01-01 | 2    
2024-01-02 | 0    
2024-01-03 | 2    
------------------
[3 cs x 1 feat]
```

`2024-01-01` and `2024-01-03` each hold 2 observations; `2024-01-02` holds none,
so it's `0`.

## Notes

- Every bucket in the contiguous grid between the data's minimum and maximum tick
  appears, even ones with zero observations.
- Like [`sum()`](sum.md), `count()` on an empty bucket is `0`, not `NULL`.

## See Also

- [`DataFrameTsResampler::sum`](sum.md), [`DataFrameTsResampler::mean`](mean.md),
  [`DataFrameTsResampler::min`](min.md), [`DataFrameTsResampler::max`](max.md),
  [`DataFrameTsResampler::first`](first.md), [`DataFrameTsResampler::last`](last.md):
  the other bucket aggregates.
- [`DataFrameTsAccessor::resample`](../dataframe-ts/resample.md): the entry point
  that produces this accessor, and the full frequency-string grammar.

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
