# `CategoryIndexer::features`

## API Signature

- `CategoryIndexer::features(names: RangeView::Of<std::string_view>) -> CategoryIndexer&`
- `CategoryIndexer::features(names: std::initializer_list<std::string_view>) -> CategoryIndexer&`

## Notes

Instance method on `self`.

## Inputs

- `names` (`RangeView::Of<std::string_view> | std::initializer_list<std::string_view>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `CategoryIndexer&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = CategoryIndexer::features(/* names */);
out.info("features", value);
```

**Expected output**

```text
features returned CategoryIndexer&
```

### Example 2 (Demo Pattern)

```cpp
    const auto split = nja::holdout_split(frame, nja::HoldoutSplitOptions{
                                                     .test_size = 0.2,
                                                     .random_state = 42,
                                                     .deterministic = true,
                                                     .shuffle = true,
                                                 });
    const DataFrame train_x = split.train_set.features(
        {"season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition", "temp", "atemp", "humidity", "windspeed"});
    const DataFrame test_x = split.test_set.features(
        {"season", "year", "month", "hour", "is_holiday", "weekday", "is_workingday", "weather_condition", "temp", "atemp", "humidity", "windspeed"});
    const Series train_y = split.train_set.features("total_count").as_series();
    const Series test_y = split.test_set.features("total_count").as_series();

    TrainOptions train_options{};
    train_options.max_iterations = 2500;
    train_options.tolerance = 1e-8;
    train_options.random.seed = 7;
    train_options.random.deterministic = true;
```

**Expected output**

```text
CV objective metric: rmse
candidate=linear_ridge objective=102.872 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[102.87158] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.41650024] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[77.611594]
candidate=decision_tree objective=44.9727 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[44.972693] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.88843998] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[27.61723]
candidate=random_forest objective=81.1235 cv_rmse=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[81.123538] cv_r2=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.63710967] cv_mae=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
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
