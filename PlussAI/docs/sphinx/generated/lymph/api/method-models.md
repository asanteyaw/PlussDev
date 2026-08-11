# `models`

## API Signature

- `models(builder: const Builder&) -> Models`

## Notes

Namespace-level API function.

## Inputs

- `builder` (`const Builder&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Models`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = models(/* builder */);
out.info("models", value);
```

**Expected output**

```text
models returned Models
```

### Example 2 (Demo Pattern)

```cpp
    cv_options.plan = CVPlanKind::CrossVal;
    cv_options.folds = 5;
    cv_options.shuffle = true;
    cv_options.random_state = 73;
    cv_options.deterministic = true;

    std::vector<nja::selection::Candidate> candidates{};
    {
      nja::Model wf = nja::models::linear_regression(LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      });
      wf.simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median});
      wf.standard_scaler();
      candidates.emplace_back("linear_ridge", wf);
    }
    {
      nja::Model wf = nja::models::decision_tree_regressor(TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
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
                  }),
                  lymph::workflow::stage::standard_scaler(),
              },
              {"median_income"}),
          lymph::workflow::feature::one_hot_encoder(lymph::prep::OneHotEncoderOptions{}, categorical),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  auto reg = lymph::models::linear_regression(pluss::ai::nodej::LinearFamilyOptions{
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
