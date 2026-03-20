# `Candidate::Candidate`

## API Signature

- `name(arg1: std::move(in_name)), arg2: workflow(std::move(in_workflow)) -> Candidate(std::string in_name, Workflow in_workflow) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`std::move(in_name))`)
  Method argument; see signature type for accepted form.
- `arg2` (`workflow(std::move(in_workflow)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Candidate(std::string in_name, Workflow in_workflow) :`.

## Options

Option semantics used by this method:

- `Candidate` (`struct`):
  - `std::string name{}`
    Option field used by this method.
  - `Workflow workflow{}`
    Option field used by this method.
  - `Candidate& operator=(Candidate&&) noexcept = default`
    Option field used by this method.
  - `Candidate& operator=(const Candidate&) = delete`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = name(/* arg1 */, /* arg2 */);
out.info("Candidate", value);
```

**Expected output**

```text
Candidate returned Candidate(std::string in_name, Workflow in_workflow) :
```

### Example 2 (Demo Pattern)

```cpp
    nja::CrossValidationOptions cv_options{};
    cv_options.plan = CVPlanKind::KFold;
    cv_options.folds = 5;
    cv_options.shuffle = true;
    cv_options.random_state = 73;
    cv_options.deterministic = true;

    std::vector<nja::selection::Candidate> candidates{};
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
```

**Expected output**

```text
Candidate returned Candidate(std::string in_name, Workflow in_workflow) :
```
