# `Workflow`

## API Signature

- `model_(arg1: std::move(model)), arg2: policy_(std::move(policy)) -> Workflow(Model model, Policy policy) :`

## Notes

Namespace-level API function.

## Inputs

- `arg1` (`std::move(model))`)
  Method argument; see signature type for accepted form.
- `arg2` (`policy_(std::move(policy)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Workflow(Model model, Policy policy) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = model_(/* arg1 */, /* arg2 */);
out.info("Workflow", value);
```

**Expected output**

```text
Workflow returned Workflow(Model model, Policy policy) :
```

### Example 2 (Demo Pattern)

```cpp
        "temp_humidity_interaction", "temp_windspeed_interaction", "atemp_humidity_interaction", "atemp_windspeed_interaction",
        "temp_minus_atemp", "hour_centered", "hour_centered_sq", "month_centered", "month_centered_sq", "weather_sq",
    });
    const Series train_y = holdout.train_set.features("total_count").as_series();
    const Series test_y = holdout.test_set.features("total_count").as_series();

    //========== standalone lymph preprocessing API ========
    // This section demonstrates direct prep estimators (learn/map/learn_map)
    // without going through Workflow composition.
    auto simple_imp = lymph::prep::simple_imputer(lymph::prep::SimpleImputerOptions{
        .strategy = lymph::prep::SimpleImputerStrategy::Median,
    });
    const DataFrame train_simple_imputed = simple_imp.learn_map(train_x);
    const DataFrame test_simple_imputed = simple_imp.map(test_x);
    const auto simple_imp_summary = simple_imp.summary();

    auto knn_imp = lymph::prep::neighbor_imputer(lymph::prep::NeighborImputerOptions{
        .neighbors = 7,
        .weighting = lymph::prep::NeighborImputerWeighting::Distance,
        .distance_floor = 1e-12,
        .empty_feature_fallback = 0.0,
```

**Expected output**

```text
Workflow returned Workflow(Model model, Policy policy) :
```
