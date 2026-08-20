# `FeatureBatch::defined`

## API Signature

- `FeatureBatch::defined() -> bool`

## Notes

Boolean state check for object readiness.

## Inputs

None.

## Output

- Returns `bool`.

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
auto value = FeatureBatch::defined();
out.info("defined", value);
```

**Expected output**

```text
defined: true
```

### Example 2 (Demo Pattern)

```cpp
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
    TrainOptions warm_opts{};
    warm_opts.max_iterations = 80;

    OnlineLinearClassifier model = pluss::ai::lymph::online::linear_classifier(online_options);
    model.learn_(warm, warm_opts).advance_(stream);

    const auto metrics = model.assess(full);
    const Series metric_values = metrics.accuracy.defined() ? metrics.accuracy : metrics.values;

    std::cout << "lymph online seen_samples=" << model.seen_samples() << '\n';
    std::cout << "lymph online metrics [accuracy]: ";
    print_vec(metric_values);
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "lymph_online_demo failed: " << ex.what() << '\n';
    return 1;
  }
}
```

**Expected output**

```text
lymph online seen_samples=1660
lymph online metrics [accuracy]: 0.9875
```
