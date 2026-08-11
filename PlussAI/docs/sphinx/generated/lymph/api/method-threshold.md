# `threshold`

## API Signature

- `threshold(scores: Series, labels: Series) -> Threshold`

## Notes

Namespace-level API function.

## Inputs

- `labels` (`Series`)
  Target values aligned to input observations.
- `scores` (`Series`)
  Metric selection or score-control argument for evaluation/tuning.

## Output

- Returns `Threshold`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = threshold(/* scores */, /* labels */);
out.info("threshold", value);
```

**Expected output**

```text
threshold returned Threshold
```

### Example 2 (Demo Pattern)

```cpp
      .mode = PowerMapMode::YeoJohnson,
      .standardize = true,
  });
  auto pmap_or = pmap.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(pmap_or, "PowerMap.learn_map")) return 1;
  print_shape("power-map", pmap_or.value());

  ThresholdBinarizer bin(ThresholdBinarizerOptions{
      .threshold = 0.0,
      .low_value = 0.0,
      .high_value = 1.0,
  });
  auto bin_or = bin.learn_map(FeatureBatch{.x = x_z, .layout = ObservationLayout::Rows});
  if (!require_ok(bin_or, "ThresholdBinarizer.learn_map")) return 1;
  print_shape("binarized", bin_or.value());

  PolynomialLift poly(PolynomialLiftOptions{
      .degree = 2,
      .interactions_only = false,
      .include_bias = true,
  });
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
