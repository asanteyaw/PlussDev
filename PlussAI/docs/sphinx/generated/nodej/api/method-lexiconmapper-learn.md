# `LexiconMapper::learn`

## API Signature

- `LexiconMapper::learn(documents: std::initializer_list<std::string>) -> Status`
- `LexiconMapper::learn(documents: RangeView::Of<std::string>) -> Status`

## Notes

Fits model/transform state from the provided training data. Call this before inference-oriented methods.

## Inputs

- `documents` (`RangeView::Of<std::string> | std::initializer_list<std::string>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = LexiconMapper::learn(/* documents */);
if (!st.ok()) {
  out.error("learn failed", st.message);
} else {
  out.info("learn ok");
}
```

**Expected output**

```text
learn: ok
status.ok() == true
```

### Example 2 (Demo Pattern)

```cpp
  auto bounded_or = make_classifier(linear_logistic_spec());
  if (!require_ok(bounded_or, "make_classifier(linear bounded)")) return 1;
  auto bounded = bounded_or.move_value();

  TrainOptions bounded_train = train;
  bounded_train.parameter_lower_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
  bounded_train.parameter_upper_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
  if (!bounded->learn(batch, bounded_train).ok()) {
    std::cerr << "bounded linear learn failed\n";
    return 1;
  }

  auto bounded_scores_or = bounded->infer_scores(batch.features);
  if (!require_ok(bounded_scores_or, "infer_scores(bounded)")) return 1;
  std::cout << "max |bounded score|: " << bounded_scores_or.value().abs().max().item<double>() << "\n";

  std::cout << "NodeJ CV + tune demo completed.\n";
  return 0;
}
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
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      },
  };

  auto reg_or = make_regressor(spec);
  if (!require_ok(reg_or, "make_regressor")) return 1;
  auto reg = reg_or.move_value();
  Status st = reg->learn(batch);
  if (!st.ok()) {
    std::cerr << "learn failed: " << st.message << "\n";
    return 1;
  }

  auto metrics_or = reg->assess(batch, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!require_ok(metrics_or, "assess")) return 1;
  print_metrics("linear metrics [mse, r2]", metrics_or.value());

  std::cout << "NodeJ DataFrame bridge demo completed.\n";
  return 0;
}
```

**Expected output**

```text
NodeJ DataFrame bridge demo
adapted batch: samples=6 features=4 weighted=yes
linear metrics [mse, r2]: 1.51681e-17 1
NodeJ DataFrame bridge demo completed.
```
