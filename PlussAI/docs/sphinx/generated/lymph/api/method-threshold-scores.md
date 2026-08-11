# `Threshold::scores`

## API Signature

- `Threshold::scores() -> const Series&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const Series&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Threshold::scores();
out.info("scores", value);
```

**Expected output**

```text
scores returned const Series&
```

### Example 2 (Demo Pattern)

```cpp
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
  }
  std::cout << "metrics [accuracy, f1, logloss]: ";
  print_vec(metric_or.value());

  auto flow_or = pluss::ai::nodej::make_flow(spec);
```

**Expected output**

```text
scores shape: [12, 3]
metrics [accuracy, f1, logloss]: 1 1 1.24859e-10
flow metrics [accuracy, f1, logloss]: 1 1 1.48016e-09
```

### Example 3 (Demo Pattern)

```cpp
  const auto rank_or = rank_features(class_batch, FeatureRankOptions{
                                                      .mode = FeatureScoreMode::ClassF,
                                                      .absolute_score = true,
                                                  });
  if (!rank_or.ok()) {
    std::cerr << rank_or.status().message << "\n";
    return 1;
  }
  std::cout << "class scores: " << rank_or.value().scores << "\n";
  std::cout << "class rank: " << rank_or.value().ranked_feature_indices << "\n";

  const auto pick_or = pick_features(class_batch, FeaturePickOptions{
                                                      .mode = FeatureScoreMode::ClassF,
                                                      .top_k = 2,
                                                      .fraction = 0.0,
                                                      .absolute_score = true,
                                                  });
  if (!pick_or.ok()) {
    std::cerr << pick_or.status().message << "\n";
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
