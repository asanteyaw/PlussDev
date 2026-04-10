# `prep::StandardScaler::learn_map`

## API Signature

- `prep::StandardScaler::learn_map(frame: const pluss::table::DataFrame&, learn_options: const TrainOptions&, infer_options: const InferOptions&) -> pluss::table::DataFrame`

## Notes

Fits model/transform state from the provided training data. Call this before inference-oriented methods.

## Inputs

- `frame` (`const pluss::table::DataFrame&`)
  Input feature data used by this call.
- `infer_options` (`const InferOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `learn_options` (`const TrainOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `pluss::table::DataFrame`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = prep::StandardScaler::learn_map(/* frame */, /* learn_options */, /* infer_options */);
out.info("learn_map", value);
```

**Expected output**

```text
learn_map returned pluss::table::DataFrame
```

### Example 2 (Demo Pattern)

```cpp
  if (!require_ok(fold_or, "fold_plan")) return 1;
  std::cout << "kfold folds=" << fold_or.value().folds.size() << "\n";

  SimpleImputer imputer(SimpleImputerOptions{
      .strategy = SimpleImputerStrategy::Median,
      .constant_value = 0.0,
      .empty_feature_fallback = 0.0,
  });
  auto imp_or = imputer.learn_map(dense);
  if (!require_ok(imp_or, "SimpleImputer.learn_map")) return 1;
  const Series x_imputed = imp_or.move_value();
  print_shape("imputed", x_imputed);

  StandardScaler zscore{};
  auto z_or = zscore.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
  if (!require_ok(z_or, "StandardScaler.learn_map")) return 1;
  const Series x_z = z_or.move_value();
  print_shape("standardized", x_z);

  MinMaxScaler minmax{};
  auto mm_or = minmax.learn_map(FeatureBatch{.x = x_imputed, .layout = ObservationLayout::Rows});
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

### Example 3 (Demo Pattern)

```cpp
  std::cout << "numeric features: ";
  for (const auto& n : numeric) std::cout << n << " ";
  std::cout << "\n";
  std::cout << "categorical features: ";
  for (const auto& n : categorical) std::cout << n << " ";
  std::cout << "\n";
  auto ord = lymph::prep::OrdinalEncoder({.include_unknown_bucket = true, .one_based = true});
  ord.features({"ocean_proximity"});
  const auto ocean_ord = ord.learn_map(housing.features({"ocean_proximity"}));
  auto lbl = lymph::prep::LabelEncoder({.include_unknown_bucket = true, .one_based = false});
  const auto ocean_lbl = lbl.learn_map(housing.features("ocean_proximity"));
  std::cout << "ordinal mapped names: ";
  for (const auto& name : ord.mapped_names_) std::cout << name << " ";
  std::cout << "\n";
  std::cout << "label mapped names: ";
  for (const auto& name : lbl.mapped_names_) std::cout << name << " ";
  std::cout << "\n";
  std::cout << "ordinal sample: " << ocean_ord << "\n";
  std::cout << "label sample: " << ocean_lbl << "\n";
  std::cout << "pred: " << pred << "\n";
  std::cout << "mse: " << metrics.mse << " r2: " << metrics.r2 << "\n";
```

**Expected output**

```text
numeric features: median_income target
categorical features: ocean_proximity
ordinal mapped names: ocean_proximity_ordinal
label mapped names: ocean_proximity_label
ordinal sample: ======================
     | ocean_proximity
----------------------
0    | 2
1    | 1
2    | 2
3    | 1
4    | 2
...
```
