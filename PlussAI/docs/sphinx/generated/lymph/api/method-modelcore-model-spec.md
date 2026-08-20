# `ModelCore::model_spec`

## API Signature

- `ModelCore::model_spec() -> const std::optional<ModelSpec>&`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `const std::optional<ModelSpec>&`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore::model_spec();
out.info("model_spec", value);
```

**Expected output**

```text
model_spec returned const std::optional<ModelSpec>&
```

### Example 2 (Demo Pattern)

```cpp
  cv_options.plan = pluss::ai::nodej::CVPlanKind::CrossVal;
  cv_options.folds = 3;
  cv_options.shuffle = true;
  cv_options.random_state = 42;
  cv_options.deterministic = true;

  const auto t_cv0 = Clock::now();
  const auto cv_plan = lymph::validate::cross(cv_options);
  const auto linear_cv = cv_plan.run(linear_wf.model_spec().value(), train_prepared, train_y, train_options, assess_options);
  const auto tree_cv = cv_plan.run(tree_wf.model_spec().value(), train_prepared, train_y, train_options, assess_options);
  const auto forest_cv = cv_plan.run(forest_wf.model_spec().value(), train_prepared, train_y, train_options, assess_options);
  const auto t_cv1 = Clock::now();
  out.writef("cross_validate block seconds={}", secs(t_cv0, t_cv1)).endln();

  out.writeln("linear cv", linear_cv.mean.rmse, linear_cv.mean.r2);
  out.writeln("tree cv", tree_cv.mean.rmse, tree_cv.mean.r2);
  out.writeln("forest cv", forest_cv.mean.rmse, forest_cv.mean.r2);
  //========== End Cell 12 ========

  //========== Cell 13: Tune Candidate Families ========
  // Equivalent in spirit to Grid/Random search over model hyperparameters.
```

**Expected output**

```text
model_spec returned const std::optional<ModelSpec>&
```
