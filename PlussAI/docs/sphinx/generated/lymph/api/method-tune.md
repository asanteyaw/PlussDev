# `tune`

## API Signature

- `tune(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`
- `tune(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`
- `tune(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`
- `tune(axes: const std::vector<GridAxis>&, arg2: const std::function<ModelSpec(const GridPoint&) -> SpecTuneResult`

## Notes

Runs parameter search and returns per-trial metrics plus best-trial selection.

## Inputs

- `arg2` (`const std::function<ModelSpec(const GridPoint&`)
  Method argument; see signature type for accepted form.
- `axes` (`const std::vector<GridAxis>&`)
  Input feature data used by this call.

## Output

- Returns `SpecTuneResult`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = tune(/* axes */, /* arg2 */);
out.info("tune", value);
```

**Expected output**

```text
tune returned SpecTuneResult
```

### Example 2 (Demo Pattern)

```cpp
      },
      .y = pluss::make_series(std::move(yv), {n}, f64()),
  };
}

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
