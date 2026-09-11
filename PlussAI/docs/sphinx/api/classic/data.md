# `pluss::ai::classic::data`

`data` owns the boundary between "a table with named columns" and "the
numeric matrix an estimator actually fits or predicts from". Every `learn` /
`infer` / `assess` signature across `classic` ultimately takes a
[`FeatureBatch`](data/FeatureBatch.md) or a
[`LabeledBatch`](data/LabeledBatch.md) — this family is where those are
defined and where a `pluss::table::DataFrame` is turned into one.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`FeatureBatch`](data/FeatureBatch.md) | the unlabeled input: a feature matrix `Series` plus its row/column layout |
| [`LabeledBatch`](data/LabeledBatch.md) | a `FeatureBatch` plus a target `Series` and optional sample weights |
| [`DataFrameBatchOptions`](data/DataFrameBatchOptions.md) | which `DataFrame` columns are features / label / weights, and how to handle temporal and text columns |
| [`feature_batch_from_dataframe`](data/feature_batch_from_dataframe.md) | `DataFrame` -> `FeatureBatch`, with the `Factorize` category-map fit/reuse contract |
| [`labeled_batch_from_dataframe`](data/labeled_batch_from_dataframe.md) | `DataFrame` -> `LabeledBatch` — the same resolution, plus a label |
| [`CategoryMap`](data/CategoryMap.md) | the fitted code table a `Factorize` column produces and reuses |

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

cls::DataFrameBatchOptions options{};
options.feature_names = {"income", "balance"};
options.label_feature = "default";

cls::StatusOr<cls::LabeledBatch> batch = cls::labeled_batch_from_dataframe(frame, options);
if (batch.ok()) {
  classifier->learn(batch.value());
}
```

For a `Series` you already hold — no `DataFrame` in the picture — build a
`FeatureBatch` / `LabeledBatch` directly by assigning `.x` / `.y` (and
`.layout` if it is not row-major); see each type's own page.

## Related Pages

- [`core`](core.md) — `TrainOptions` / `InferOptions` / `AssessOptions`, the
  configuration that travels alongside a batch into every verb.
- [`estimators`](estimators.md) — the `learn` / `infer` / `assess` signatures
  that consume these batches.
- [`prep::encoding`](prep/encoding.md) — when a one-shot `Factorize` / `Hash`
  in `DataFrameBatchOptions` is not enough for a categorical column.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

data/Introduction
data/FeatureBatch
data/LabeledBatch
data/DataFrameBatchOptions
data/feature_batch_from_dataframe
data/labeled_batch_from_dataframe
data/CategoryMap
```
<!-- apiref:toctree:end -->
