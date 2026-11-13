<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98f31c4a59f7 -->

# DataFrameTsResampler::asfreq

[Back to DataFrameTsResampler API Reference](../../dataframe_ts_resampler_api_reference)

## Signature

```cpp
DataFrame DataFrameTsResampler::asfreq(std::optional<double> fill_value = std::nullopt) const
```

## What It Does

Reindexes the original (unaggregated) rows onto the regular grid produced by
[`DataFrameTsAccessor::resample()`](../dataframe-ts/resample.md), using
[`TsAlignMode::Exact`](../dataframe-ts/align_to.md) tick matching — a grid tick
keeps its source value only if a source observation lands on that exact tick;
otherwise it's `NULL`. This is unlike [`mean()`](mean.md)/[`sum()`](sum.md)/etc.,
which aggregate every observation *within* each bucket; `asfreq()` only ever
keeps a value that already sat precisely on a bucket boundary.

If `fill_value` is supplied, every grid tick that did **not** get an exact match
is replaced by that single constant instead of `NULL` — the same constant for
every gap, regardless of position.

## Parameters

**fill_value** (`std::optional<double>`, default `std::nullopt`)

A constant substituted for every non-exact-match grid tick. Leave as
`std::nullopt` to keep those ticks `NULL`.

## Returns

`DataFrame`
    One row per tick in the regular grid, with only exact matches (or the
    supplied `fill_value`) populated.

## Examples

Example 1. Reindex a 3-point series onto its own daily grid.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Slate(pluss::make_series({100.0, 102.0, 110.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
std::cout << readings;

auto out = readings.ts().resample("D").asfreq();
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
2024-01-03 | NULL 
2024-01-04 | NULL 
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

`resample("D")` builds a 5-tick grid spanning `2024-01-01` through `2024-01-05`
(the source's own min/max ticks). `2024-01-03` and `2024-01-04` have no source
observation on them, so they're `NULL`.

Example 2. Same reindex, with a fill value for the gaps.

```cpp
auto out2 = readings.ts().resample("D").asfreq(0.0);
std::cout << out2;
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | 0    
2024-01-04 | 0    
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

The exact matches (`100`, `102`, `110`) are unaffected; both gap ticks are
replaced with the supplied `0.0`.

## Notes

- `asfreq()` is a thin wrapper over
  [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md) with
  `TsAlignMode::Exact` — see that page for the full tick-matching contract.
- Unlike [`ffill()`](ffill.md)/[`bfill()`](bfill.md)/[`nearest()`](nearest.md),
  `asfreq()` never borrows a neighboring observation's value — a gap tick is
  either `NULL` or the single supplied `fill_value`, never a nearby real reading.

## See Also

- [`DataFrameTsResampler::ffill`](ffill.md), [`DataFrameTsResampler::bfill`](bfill.md),
  [`DataFrameTsResampler::nearest`](nearest.md): the other reindex modes, powered
  by `TsAlignMode::Backward`/`Forward`/`Nearest` respectively.
- [`DataFrameTsAccessor::align_to`](../dataframe-ts/align_to.md): the underlying
  operation and its four alignment modes.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
