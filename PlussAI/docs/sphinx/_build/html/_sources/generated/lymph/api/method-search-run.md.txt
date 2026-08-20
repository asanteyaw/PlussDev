# `search::run`

## API Signature

- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`
- `search::run(arg2: const std::function<Model(const GridPoint&) -> Result`

## Notes

Instance method on `self`.

## Inputs

- `arg2` (`const std::function<Model(const GridPoint&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Result`.

## Options

Option semantics used by this method:

- `search` (`struct`):
  - `selection::CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = search::run(/* arg2 */);
out.info("run", value);
```

**Expected output**

```text
run returned Result
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
run returned Result
```
