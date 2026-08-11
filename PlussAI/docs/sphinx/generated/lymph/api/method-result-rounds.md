# `Result::rounds`

## API Signature

- `Result::rounds() -> std::int64_t`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::int64_t`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::rounds();
out.info("rounds", value);
```

**Expected output**

```text
rounds: <numeric_result>
```

### Example 2 (Demo Pattern)

```cpp
  const Series x_rb = rb_or.move_value();
  print_shape("robust", x_rb);

  NeighborImputer knn_imp(NeighborImputerOptions{.neighbors = 2});
  auto knn_or = knn_imp.learn_map(dense);
  if (!require_ok(knn_or, "NeighborImputer.learn_map")) return 1;
  print_shape("neighbor-imputed", knn_or.value());

  IterativeImputer it_imp(IterativeImputerOptions{.rounds = 6, .ridge = 1e-8});
  auto it_or = it_imp.learn_map(dense);
  if (!require_ok(it_or, "IterativeImputer.learn_map")) return 1;
  print_shape("iterative-imputed", it_or.value());

  VectorNormalizer norm(VectorNormalizerOptions{.norm = VectorNorm::L2});
  auto norm_or = norm.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(norm_or, "VectorNormalizer.learn_map")) return 1;
  print_shape("vector-normalized", norm_or.value());

  MaxAbsScaler maxabs{};
  auto maxabs_or = maxabs.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(maxabs_or, "MaxAbsScaler.learn_map")) return 1;
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
