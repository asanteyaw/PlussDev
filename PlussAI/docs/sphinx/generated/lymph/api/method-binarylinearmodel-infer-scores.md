# `BinaryLinearModel::infer_scores`

## API Signature

- `BinaryLinearModel::infer_scores(x: const Csr&) -> Series`
- `BinaryLinearModel::infer_scores(x: const Triplet&) -> Series`

## Notes

Computes predictions from prepared inputs using the currently learned state.

## Inputs

- `x` (`const Csr& | const Triplet&`)
  Input feature data used by this call.

## Output

- Returns `Series`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = BinaryLinearModel::infer_scores(/* x */);
out.info("infer_scores", value);
```

**Expected output**

```text
infer_scores returned Series
```

### Example 2 (Demo Pattern)

```cpp
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
  }
  auto cls = cls_or.move_value();
  const auto learn_status = cls->learn(batch);
  if (!learn_status.ok()) {
    std::cerr << "learn failed: " << learn_status.message << '\n';
    return 1;
  }

  auto score_or = cls->infer_scores(batch.features);
  if (!score_or.ok()) {
    std::cerr << "infer_scores failed: " << score_or.status().message << '\n';
    return 1;
  }
  std::cout << "scores shape: [" << score_or.value().sizes()[0] << ", " << score_or.value().sizes()[1] << "]\n";

  pluss::ai::nodej::AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::F1, MetricKind::LogLoss};
  auto metric_or = cls->assess(batch, assess);
  if (!metric_or.ok()) {
    std::cerr << "assess failed: " << metric_or.status().message << '\n';
    return 1;
```

**Expected output**

```text
scores shape: [12, 3]
metrics [accuracy, f1, logloss]: 1 1 1.24859e-10
flow metrics [accuracy, f1, logloss]: 1 1 1.48016e-09
```
