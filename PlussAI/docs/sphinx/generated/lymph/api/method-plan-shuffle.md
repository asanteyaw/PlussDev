# `Plan::shuffle`

## API Signature

- `Plan::shuffle(value: bool = true) -> Plan&`

## Notes

Instance method on `self`.

## Inputs

- `value` (`bool`), default: `true`
  Method argument; see signature type for accepted form.

## Output

- Returns `Plan&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Plan::shuffle(/* value */);
out.info("shuffle", value);
```

**Expected output**

```text
shuffle returned Plan&
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
      MetricKind::Precision,
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
              .query_mode = NeighborQueryMode::KNearest,
              .distance_weighted = true,
          },
      },
  };
  SplitOptions split{};
  split.plan = CVPlanKind::CrossVal;
  split.folds = 3;
  split.shuffle = true;
  split.random.seed = 23;
  AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::F1};
  TrainOptions train{};
  train.max_iterations = 600;
  train.tolerance = 1e-7;

  const auto tune_or = sweep(specs, class_batch, split, train, assess, MetricKind::Accuracy, TuneObjectivePolicy::Maximize);
  if (!tune_or.ok()) {
    std::cerr << tune_or.status().message << "\n";
    return 1;
  }
```

**Expected output**

```text
class scores: pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 13.5, 0]
class rank: pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 2]
picked feature ids: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 0]
picked shape: 6x2
regression picked ids: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
dense objective: pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.5]
dense ranks: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
...
```
