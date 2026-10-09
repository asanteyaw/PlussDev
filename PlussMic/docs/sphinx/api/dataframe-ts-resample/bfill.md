<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cf1c9846acee -->

# DataFrameTsResampler::bfill

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrameTsResampler::bfill() const -> DataFrame
```

## What It Does

Reindexes the original (unaggregated) rows onto the regular grid produced by
[`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md), using
[`TsAlignMode::Forward`](../dataframe-ts/align_to.md) tick matching — each grid
tick takes the value of the next source observation at or after it (the
standard "backward-fill" reindex, so named because values flow backward in
time to fill earlier gaps). A tick with no eligible later observation produces
`NULL`.

## Returns

`DataFrame`
    One row per tick in the regular grid, each carrying back the next known
    value at or after that tick.

## Examples

Example 1. Backward-fill a 3-point series onto its own daily grid.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Slate(pluss::make_series({100.0, 102.0, 110.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").bfill();
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
2024-01-03 | 110  
2024-01-04 | 110  
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

`2024-01-03` and `2024-01-04` both take `110` (the value observed on
`2024-01-05`, the next tick at or after each of them); `2024-01-01` and
`2024-01-02` have their own exact observations, `100` and `102`.

## Notes

- `bfill()` is a thin wrapper over
  [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md) with
  `TsAlignMode::Forward` — see that page for the full asof-matching contract.
- A grid tick later than every source observation has no later value to borrow
  and produces `NULL`.

## See Also

- [`DataFrameTsResampler::ffill`](ffill.md), [`DataFrameTsResampler::asfreq`](asfreq.md),
  [`DataFrameTsResampler::nearest`](nearest.md): the other reindex modes, powered
  by `TsAlignMode::Backward`/`Exact`/`Nearest` respectively.
- [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md): the underlying
  operation and its four alignment modes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asfreq.html">
    <span class="label">Previous</span>
    <span class="title">asfreq</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
