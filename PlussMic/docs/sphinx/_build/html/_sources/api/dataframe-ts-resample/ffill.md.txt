<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 85e40bacf693 -->

# DataFrameTsResampler::ffill

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrameTsResampler::ffill() const -> DataFrame
```

## What It Does

Reindexes the original (unaggregated) rows onto the regular grid produced by
[`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md), using
[`TsAlignMode::Backward`](../dataframe-ts/align_to.md) tick matching — each grid
tick takes the value of the most recent source observation at or before it (the
standard "forward-fill" reindex). A tick with no eligible prior observation
produces `NULL`.

## Returns

`DataFrame`
    One row per tick in the regular grid, each carrying forward the last known
    value at or before that tick.

## Examples

Example 1. Forward-fill a 3-point series onto its own daily grid.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({100.0, 102.0, 110.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").ffill();
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
2024-01-04 | 102  
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

`2024-01-03` and `2024-01-04` both carry forward `102` (the value observed on
`2024-01-02`, the most recent tick at or before each of them); `2024-01-05`
has its own exact observation, `110`.

## Notes

- `ffill()` is a thin wrapper over
  [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md) with
  `TsAlignMode::Backward` — see that page for the full asof-matching contract.
- A grid tick earlier than every source observation has no prior value to carry
  forward and produces `NULL`.

## See Also

- [`DataFrameTsResampler::bfill`](bfill.md), [`DataFrameTsResampler::asfreq`](asfreq.md),
  [`DataFrameTsResampler::nearest`](nearest.md): the other reindex modes, powered
  by `TsAlignMode::Forward`/`Exact`/`Nearest` respectively.
- [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md): the underlying
  operation and its four alignment modes.

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
