# `WorkflowModel::workflow`

## API Signature

- `WorkflowModel::workflow() -> const Workflow&`
- `WorkflowModel::workflow() -> Workflow&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `Workflow&`.
- Returns `const Workflow&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = WorkflowModel::workflow();
out.info("workflow", value);
```

**Expected output**

```text
workflow returned const Workflow&
```

### Example 2 (Demo Pattern)

```cpp
    {
      auto wf = nja::Workflow{
          .model = nja::linear_factory::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      }),
          .stages = {
              nja::workflow::stage::simple_imputer(nja::prep::SimpleImputerOptions{.strategy = nja::prep::SimpleImputerStrategy::Median}),
              nja::workflow::stage::standard_scaler(),
          },
      };
      candidates.emplace_back("linear_ridge", std::move(wf));
    }
    {
      auto wf = nja::Workflow{
          .model = nja::tree_factory::decision_tree_regressor(TreeFamilyOptions{          .criterion = TreeCriterion::Mse,
          .max_depth = 18,
          .min_samples_leaf = 8,
          .min_samples_split = 24,
      }),
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
  data_options.feature_names = {"median_income", "ocean_proximity", "target"};
  data_options.label_feature = "target";
  data_options.note_policy = pluss::ai::nodej::DataNotePolicy::Hash;
  data_options.layout = pluss::ai::nodej::ObservationLayout::Rows;

  auto preprocessing = lymph::make_feature_transformer(
      housing,
      data_options,
      std::vector<lymph::workflow::feature::Branch>{
          lymph::workflow::feature::compose(
              {
                  lymph::workflow::stage::simple_imputer(lymph::prep::SimpleImputerOptions{
                      .strategy = lymph::prep::SimpleImputerStrategy::Mean,
                  }),
                  lymph::workflow::stage::standard_scaler(),
              },
              {"median_income"}),
          lymph::workflow::feature::one_hot_encoder(lymph::prep::OneHotEncoderOptions{}, categorical),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);
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
