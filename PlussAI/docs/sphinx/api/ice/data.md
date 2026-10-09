# `pluss::ai::ice::data`

`data` owns the dataset layer `models_ts`/`models_nlp`/`models_rl`
build on: a `Layout`-tagged, categorical-aware
[`data::Batch`](data/Batch.md) (distinct from the core
[`ice::Batch`](step/Batch.md) the training loop itself feeds to
`Model::forward`/`compute_loss` -- see
[`data`'s own Introduction](data/Introduction.md#two-batch-types)),
the generic [`DataSet`](data/DataSet.md) that materializes one from a
`DataFrame` + [`DataSpec`](data/DataSpec.md), and the windowed-layout
convenience wrapper [`TimeSeriesDataSet`](data/TimeSeriesDataSet.md)
every `models_ts` entry's own `*Batch` type is built from.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`FieldSpec`](data/FieldSpec.md) | a named variable's `Role` (static/known/observed/feature x continuous/categorical) within a `DataSpec`'s field list -- also the `VariableSpec` alias in `TimeSeriesDataSetOptions` |
| [`WindowSpec`](data/WindowSpec.md) | lookback/horizon lengths for a windowed `DataSpec` |
| [`CategoricalVariable`](data/CategoricalVariable.md) | marks one slot within a covariate group as categorical, and its cardinality |
| [`CategoricalVocabulary`](data/CategoricalVocabulary.md) | a fitted variable-name-to-category-strings mapping, with a trailing unknown slot |
| [`Batch`](data/Batch.md) | the `Layout`-tagged, categorical-aware batch `DataSet`/`TimeSeriesDataSet` materialize |
| [`DataSpec`](data/DataSpec.md) | layout, time/group keys, target, field roles, window sizing for `DataSet::from_frame` |
| [`DataSet`](data/DataSet.md) | the generic dataset: `DataFrame`/`Batch` in, materialized `Batch` + fitted vocabularies + a `Source` out |
| [`TimeSeriesBatch`](data/TimeSeriesBatch.md) | the static/encoder/decoder-shaped batch every `models_ts` `*Batch` type converts from |
| [`TimeSeriesDataSetOptions`](data/TimeSeriesDataSetOptions.md) | time index, group keys, target, variable roles, encoder/decoder lengths for `TimeSeriesDataSet::from_dataframe` |
| [`TimeSeriesDataSet`](data/TimeSeriesDataSet.md) | the windowed-layout convenience layer: sorted windowing, gap detection, vocabulary fit/reuse |

## How To Call It

Build a time-series dataset from a dataframe, then prepare its training source. The program prints the number of windows and the number of learn batches.

Build a time-series dataset from a dataframe, then prepare its training source. The program prints the number of windows and the number of learn batches.

```cpp
#include <pluss/pluss.h>
#include <pluss/mic.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace data = ice::data;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto f32 = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const DataFrame df({
      {"group", Slate(pluss::make_note({"g0", "g0", "g0", "g0", "g1", "g1", "g1", "g1"}))},
      {"region", Slate(pluss::make_note({"north", "north", "north", "north", "south", "south", "south", "south"}))},
      {"t", Slate(pluss::make_series({0.0, 1.0, 2.0, 3.0, 0.0, 1.0, 2.0, 3.0}, f32))},
      {"y", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, f32))},
  });

  data::TimeSeriesDataSetOptions options{};
  options.time_idx = "t";
  options.group_ids = {"group"};
  options.target = "y";
  options.variables = {{"region", data::VariableKind::StaticCategorical}};
  options.encoder_length = 2;
  options.decoder_length = 1;
  options.cfg = f32;

  const auto dataset = data::TimeSeriesDataSet::from_dataframe(df, options);
  const auto source = dataset.source();
  source->prepare();
  std::cout << "windows = " << dataset.size() << "\n";
  std::cout << "learn batches = " << source->learn_batches().size() << "\n";
  return 0;
}
```

Output

```text
windows = 4
learn batches = 1
```

Output

```text
windows = 4
learn batches = 1
```

A `TimeSeriesDataSet` is the usual entry point for forecasting data;
reach for the generic [`DataSet`](data/DataSet.md) directly for
`CrossSection` data (classification/regression with no time
dimension), or when building a `*Batch` by hand for a `models_ts`
entry that does not need the windowing/vocabulary machinery.

## Related Pages

- [`source`](source.md) -- `Source`/`DataSetSource`, what
  [`DataSet::source`](data/DataSet/source.md)/
  [`TimeSeriesDataSet::source`](data/TimeSeriesDataSet/source.md)
  build.
- [`step`](step.md) -- [`Batch`](step/Batch.md), the core batch,
  distinct from [`data::Batch`](data/Batch.md) -- see
  [`data`'s own Introduction](data/Introduction.md#two-batch-types).
- [`coach`](coach.md) -- `Coach`, the usual consumer of the `Source`
  this family's types build.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

data/Introduction
data/FieldSpec
data/WindowSpec
data/CategoricalVariable
data/CategoricalVocabulary
data/Batch
data/DataSpec
data/DataSet
data/TimeSeriesBatch
data/TimeSeriesDataSetOptions
data/TimeSeriesDataSet
```
<!-- apiref:toctree:end -->
