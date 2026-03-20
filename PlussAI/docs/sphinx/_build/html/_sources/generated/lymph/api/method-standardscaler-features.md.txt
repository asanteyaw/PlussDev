# `StandardScaler::features`

## API Signature

- `StandardScaler::features(names: std::initializer_list<std::string_view>) -> StandardScaler&`

## Notes

Instance method on `self`.

## Inputs

- `names` (`std::initializer_list<std::string_view>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `StandardScaler&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = StandardScaler::features(/* names */);
out.info("features", value);
```

**Expected output**

```text
features returned StandardScaler&
```

### Example 2 (Demo Pattern)

```cpp
        .save_pdf((out_dir / "bike_demand_vs_temp.pdf").string());

    const auto split = lymph::holdout_split(frame, lymph::HoldoutSplitOptions{
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
features returned StandardScaler&
```
