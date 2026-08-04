# `FeatureBatch::sample_count`

## API Signature

- `FeatureBatch::sample_count() -> std::int64_t`

## Notes

Reports a cardinality/count statistic from the current object state.

## Inputs

None.

## Output

- Returns `std::int64_t`.

## Options

Option semantics used by this method:

- `FeatureBatch` (`struct`):
  - `Series x = Series::undefined()`
    Option field used by this method.
  - `ObservationLayout layout = ObservationLayout::Rows`
    Declares whether observations are represented by rows or columns.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = FeatureBatch::sample_count();
out.info("sample_count", value);
```

**Expected output**

```text
sample_count: <numeric_result>
```

### Example 2 (Demo Pattern)

```cpp
  options.sample_weight_feature = "weight";
  options.note_policy = DataNotePolicy::Hash;
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;

  auto batch_or = labeled_batch_from_dataframe(frame, options);
  if (!require_ok(batch_or, "labeled_batch_from_dataframe")) return 1;

  const LabeledBatch& batch = batch_or.value();
  std::cout << "adapted batch: samples=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count()
            << " weighted=" << (batch.sample_weights.defined() ? "yes" : "no") << "\n";

  ModelSpec spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      },
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.51681e-17 1
NodeJ DataFrame bridge demo completed.
```

### Example 3 (Demo Pattern)

```cpp
                                          f64());
  const FeatureBatch dense{.x = x_raw, .layout = ObservationLayout::Rows};
  const Series labels = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());

  SplitOptions split{};
  split.holdout_fraction = 0.33;
  split.random.seed = 7;

  auto holdout_or = pluss::ai::nodej::holdout_split(dense.sample_count(), split);
  if (!require_ok(holdout_or, "holdout_split")) return 1;
  std::cout << "holdout train idx count=" << holdout_or.value().train_indices.numel()
            << " test idx count=" << holdout_or.value().test_indices.numel() << "\n";

  auto strat_or = pluss::ai::nodej::strat_holdout_split(labels, split);
  if (!require_ok(strat_or, "strat_holdout_split")) return 1;
  std::cout << "strat holdout train idx count=" << strat_or.value().train_indices.numel()
            << " test idx count=" << strat_or.value().test_indices.numel() << "\n";

  split.folds = 3;
  auto fold_or = pluss::ai::nodej::crossval_plan(dense.sample_count(), split);
  if (!require_ok(fold_or, "crossval_plan")) return 1;
```

**Expected output**

```text
NodeJ split + prep demo
holdout train idx count=4 test idx count=2
strat holdout train idx count=4 test idx count=2
kfold folds=3
imputed shape=[6, 3]
standardized shape=[6, 3]
minmax shape=[6, 3]
robust shape=[6, 3]
neighbor-imputed shape=[6, 3]
iterative-imputed shape=[6, 3]
vector-normalized shape=[6, 3]
maxabs shape=[6, 3]
...
```
