# `Result::threshold`

## API Signature

- `Result::threshold() -> double`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `double`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::threshold();
out.info("threshold", value);
```

**Expected output**

```text
threshold: <numeric_result>
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
