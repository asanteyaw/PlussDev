# `Workflow::feature_transformer`

## API Signature

- `Workflow::feature_transformer(transformer: prep::FeatureTransformer) -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

- `transformer` (`prep::FeatureTransformer`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Workflow&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = Workflow::feature_transformer(/* transformer */);
out.info("feature_transformer", value);
```

**Expected output**

```text
feature_transformer returned Workflow&
```

### Example 2 (Demo Pattern)

```cpp
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  auto reg = lymph::linear_factory::linear_regression(pluss::ai::nodej::LinearFamilyOptions{
      .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
      .regularization = pluss::ai::nodej::RegularizationKind::None,
      .regularization_strength = 0.0,
      .fit_intercept = true,
  });
  reg.feature_transformer(std::move(preprocessing));
  reg.learn_(housing, data_options);

  const auto pred = reg.infer(housing, data_options);
  const auto metrics = reg.assess(housing,
                                  data_options,
                                  pluss::ai::nodej::AssessOptions{
                                      .metrics = {pluss::ai::nodej::MetricKind::Mse, pluss::ai::nodej::MetricKind::R2},
                                  });

  std::cout << "numeric features: ";
  for (const auto& n : numeric) std::cout << n << " ";
  std::cout << "\n";
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
