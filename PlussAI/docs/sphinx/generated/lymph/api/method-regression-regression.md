# `Regression::Regression`

## API Signature

- `truth_(arg1: std::move(truth)), arg2: prediction_(std::move(prediction)) -> Regression(Series truth, Series prediction) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`std::move(truth))`)
  Method argument; see signature type for accepted form.
- `arg2` (`prediction_(std::move(prediction)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Regression(Series truth, Series prediction) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = truth_(/* arg1 */, /* arg2 */);
out.info("Regression", value);
```

**Expected output**

```text
Regression returned Regression(Series truth, Series prediction) :
```

### Example 2 (Demo Pattern)

```cpp
          .gamma = 2.0,
          .epsilon = 0.0,
      },
  };
}

ModelSpec linear_reg_spec(double lambda) {
  return ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = lambda,
          .fit_intercept = true,
      },
  };
}

LabeledBatch make_reg_batch() {
  constexpr std::int64_t n = 80;
```

**Expected output**

```text
NodeJ CV + tune demo
linear mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0.4375 0.4375 0.458333 0.625 0.5 0.25 0.773477
linear std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0.108253 0.108253 0.0721688 0.216506 0.117851 0.25 0.0776902
kernel mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 1 1 1 1 1 1 0.372106
kernel std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]: 0 0 0 0 0 0 0.0311667
sweep trial objectives: 0.4375 1
sweep best index: 1
probe best index: 1
regression quantile sweep objectives: 0.0352983 0.743786
regression quantile sweep best index: 0
max |bounded score|: 0
NodeJ CV + tune demo completed.
```

### Example 3 (Demo Pattern)

```cpp
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

  auto reg_or = make_regressor(spec);
  if (!require_ok(reg_or, "make_regressor")) return 1;
  auto reg = reg_or.move_value();
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.51681e-17 1
NodeJ DataFrame bridge demo completed.
```
