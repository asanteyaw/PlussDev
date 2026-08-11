# `Classification::summary`

## API Signature

- `Classification::summary() -> ClassReport`
- `Classification::summary(options: ClassReportOptions) -> ClassReport`

## Notes

Instance method on `self`.

## Inputs

- `options` (`ClassReportOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `ClassReport`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Classification::summary();
out.info("summary", value);
```

**Expected output**

```text
metrics: <metric_1> <metric_2> ...
evaluation completed
```

### Example 2 (Demo Pattern)

```cpp
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
    });
    const DataFrame train_knn_imputed = knn_imp.learn_map(train_x);
    const DataFrame test_knn_imputed = knn_imp.map(test_x);
    const auto knn_imp_summary = knn_imp.summary();

    auto iter_imp = lymph::prep::iterative_imputer(lymph::prep::IterativeImputerOptions{
```

**Expected output**

```text
metrics: <metric_1> <metric_2> ...
evaluation completed
```
