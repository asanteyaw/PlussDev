# `HoldoutPlan::split`

## API Signature

- `HoldoutPlan::split(batch: const LabeledBatch&) -> HoldoutSplit`
- `HoldoutPlan::split(frame: const pluss::table::DataFrame&) -> HoldoutSplit`

## Notes

Builds train/test index partitions according to split-plan options.

## Inputs

- `batch` (`const LabeledBatch&`)
  Input feature data used by this call.
- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.

## Output

- Returns `HoldoutSplit`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = HoldoutPlan::split(/* batch */);
out.info("split", value);
```

**Expected output**

```text
split completed
train/test partitions ready
```

### Example 2 (Demo Pattern)

```cpp
} // namespace

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
  if (st.ok()) return true;
  std::cerr << label << " failed: " << st.message << "\n";
  return false;
}

} // namespace

int main() {
  std::cout << "NodeJ split + prep demo\n";

  const Series x_raw = pluss::make_series(std::vector<double>{
                                              1.0,  2.0, std::numeric_limits<double>::quiet_NaN(),
                                              2.0,  2.5, 1.0,
                                              3.0,  std::numeric_limits<double>::quiet_NaN(), 0.0,
                                              4.0,  3.5, 1.0,
                                              5.0,  4.0, 0.0,
                                              6.0,  4.5, 1.0},
                                          {6, 3},
                                          f64());
  const FeatureBatch dense{.x = x_raw, .layout = ObservationLayout::Rows};
  const Series labels = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
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
