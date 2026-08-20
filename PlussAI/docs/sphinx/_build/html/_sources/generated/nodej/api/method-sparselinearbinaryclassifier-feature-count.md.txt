# `SparseLinearBinaryClassifier::feature_count`

## API Signature

- `SparseLinearBinaryClassifier::feature_count() -> std::int64_t`

## Notes

Reports a cardinality/count statistic from the current object state.

## Inputs

None.

## Output

- Returns `std::int64_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = SparseLinearBinaryClassifier::feature_count();
out.info("feature_count", value);
```

**Expected output**

```text
feature_count: <numeric_result>
```

### Example 2 (Demo Pattern)

```cpp
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
  };
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
}

Config i64() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

LabeledBatch slice_rows(const LabeledBatch& batch, std::int64_t start, std::int64_t count) {
  const Series idx = pluss::series::arange(start, start + count, i64());
  const Series x = batch.features.x.index_select(0, idx).reshape({count, batch.features.feature_count()});
  const Series y = batch.y.index_select(0, idx).reshape({count});
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

LabeledBatch make_class_batch() {
  std::vector<double> xv{};
  std::vector<double> yv{};
  xv.reserve(2 * 90);
  yv.reserve(90);
```

**Expected output**

```text
online seen_samples=1872
online metrics [accuracy, logloss]: 1 0.27341
```
