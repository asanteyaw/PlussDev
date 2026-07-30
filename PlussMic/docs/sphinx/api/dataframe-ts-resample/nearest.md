<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f067a478243a -->

# DataFrameTsResampler::nearest

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrameTsResampler::nearest() const -> DataFrame
```

## What It Does

Reindexes the original (unaggregated) rows onto the regular grid produced by
[`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md), using
[`TsAlignMode::Nearest`](../dataframe-ts/align_to.md) tick matching — each grid
tick takes the value of whichever source observation is closest to it in time
(ties break toward the earlier candidate). A tick with no source observations
at all produces `NULL`.

## Returns

`DataFrame`
    One row per tick in the regular grid, each taking the closest source
    observation's value.

## Examples

Example 1. Nearest-fill a 3-point series onto its own daily grid.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({100.0, 102.0, 110.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").nearest();
std::cout << out;
```

### Output (before)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-05 | 110  
------------------
[3 cs x 1 feat]
```

### Output (after)

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
```

`2024-01-03` is 1 day from `2024-01-02` (`102`) and 2 days from `2024-01-05`
(`110`), so it takes `102`; `2024-01-04` is 2 days from `2024-01-02` and 1 day
from `2024-01-05`, so it takes `110`.

## Notes

- `nearest()` is a thin wrapper over
  [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md) with
  `TsAlignMode::Nearest` — see that page for the full asof-matching contract.
- Compare with [`ffill()`](ffill.md) (always the prior observation) and
  [`bfill()`](bfill.md) (always the next observation) — `nearest()` picks
  whichever of the two is closer, independently for every grid tick.

## See Also

- [`DataFrameTsResampler::ffill`](ffill.md), [`DataFrameTsResampler::bfill`](bfill.md),
  [`DataFrameTsResampler::asfreq`](asfreq.md): the other reindex modes, powered
  by `TsAlignMode::Backward`/`Forward`/`Exact` respectively.
- [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md): the underlying
  operation and its four alignment modes.

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
