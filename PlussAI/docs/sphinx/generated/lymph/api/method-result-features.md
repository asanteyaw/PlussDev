# `Result::features`

## API Signature

- `Result::features() -> const std::vector<std::string>&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::vector<std::string>&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Result::features();
out.info("features", value);
```

**Expected output**

```text
features returned const std::vector<std::string>&
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
  const pluss::Series y = x.mul(2.0).add(pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0, 0.0, 1.0}, {6}, f64));

  const pluss::table::DataFrame housing({
      {"median_income", pluss::table::Thread(x)},
      {"ocean_proximity", cat},
      {"target", pluss::table::Thread(y)},
  });

  const auto numeric = housing.feataxis().series().features();
  const auto categorical = housing.feataxis().note().features();

  pluss::ai::nodej::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"median_income", "ocean_proximity", "target"};
  data_options.label_feature = "target";
  data_options.note_policy = pluss::ai::nodej::DataNotePolicy::Hash;
  data_options.layout = pluss::ai::nodej::ObservationLayout::Rows;

  auto preprocessing = lymph::make_feature_transformer(
      housing,
      data_options,
      std::vector<lymph::workflow::feature::Branch>{
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
