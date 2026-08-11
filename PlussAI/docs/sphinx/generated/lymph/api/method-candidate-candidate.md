# `Candidate::Candidate`

## API Signature

- `name(arg1: std::move(in_name)), arg2: model(std::move(in_model)), arg3: train_options(std::move(in_train_options)) -> Candidate(std::string in_name, Model in_model, std::optional<TrainOptions> in_train_options = std::nullopt) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`std::move(in_name))`)
  Method argument; see signature type for accepted form.
- `arg2` (`model(std::move(in_model))`)
  Method argument; see signature type for accepted form.
- `arg3` (`train_options(std::move(in_train_options)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Candidate(std::string in_name, Model in_model, std::optional<TrainOptions> in_train_options = std::nullopt) :`.

## Options

Option semantics used by this method:

- `Candidate` (`struct`):
  - `std::string name{}`
    Option field used by this method.
  - `Model model`
    Option field used by this method.
  - `std::optional<TrainOptions> train_options{}`
    Controls fold/trial/count sizing.
  - `Candidate& operator=(Candidate&&) noexcept = default`
    Option field used by this method.
  - `Candidate& operator=(const Candidate&) noexcept = default`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = name(/* arg1 */, /* arg2 */, /* arg3 */);
out.info("Candidate", value);
```

**Expected output**

```text
Candidate returned Candidate(std::string in_name, Model in_model, std::optional<TrainOptions> in_train_options = std::nullopt) :
```

### Example 2 (Demo Pattern)

```cpp
    nja::CrossValidationOptions cv_options{};
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
