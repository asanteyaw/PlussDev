# `Plan::folds`

## API Signature

- `Plan::folds(value: std::int64_t) -> Plan&`

## Notes

Instance method on `self`.

## Inputs

- `value` (`std::int64_t`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Plan&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Plan::folds(/* value */);
out.info("folds", value);
```

**Expected output**

```text
folds returned Plan&
```

### Example 2 (Demo Pattern)

```cpp
int main() {
  std::cout << "NodeJ CV + tune demo\n";

  const LabeledBatch batch = make_xor_batch();

  SplitOptions split{};
  split.plan = CVPlanKind::CrossVal;
  split.folds = 4;
  split.shuffle = true;
  split.random.seed = 73;
  split.random.deterministic = true;

  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;

  AssessOptions assess{};
  assess.metrics = {
      MetricKind::Accuracy,
      MetricKind::BalancedAccuracy,
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
  std::cout << "holdout train idx count=" << holdout_or.value().train_indices.numel()
            << " test idx count=" << holdout_or.value().test_indices.numel() << "\n";

  auto strat_or = pluss::ai::nodej::strat_holdout_split(labels, split);
  if (!require_ok(strat_or, "strat_holdout_split")) return 1;
  std::cout << "strat holdout train idx count=" << strat_or.value().train_indices.numel()
            << " test idx count=" << strat_or.value().test_indices.numel() << "\n";

  split.folds = 3;
  auto fold_or = pluss::ai::nodej::crossval_plan(dense.sample_count(), split);
  if (!require_ok(fold_or, "crossval_plan")) return 1;
  std::cout << "kfold folds=" << fold_or.value().folds.size() << "\n";

  SimpleImputer imputer(SimpleImputerOptions{
      .strategy = SimpleImputerStrategy::Median,
      .constant_value = 0.0,
      .empty_feature_fallback = 0.0,
  });
  auto imp_or = imputer.learn_map(dense);
  if (!require_ok(imp_or, "SimpleImputer.learn_map")) return 1;
  const Series x_imputed = imp_or.move_value();
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
