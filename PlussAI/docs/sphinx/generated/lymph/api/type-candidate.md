# `Candidate`

## API Signature

- `Candidate`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `Candidate` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::string name{}`
- `Workflow workflow{}`
- `Candidate& operator=(Candidate&&) noexcept = default`
- `Candidate& operator=(const Candidate&) = delete`

## Examples

### Example 1

```cpp
  const auto t_cv1 = Clock::now();
  out.writef("cross_validate block seconds={}", secs(t_cv0, t_cv1)).endln();

  out.writeln("linear cv", linear_cv.mean.rmse, linear_cv.mean.r2);
  out.writeln("tree cv", tree_cv.mean.rmse, tree_cv.mean.r2);
  out.writeln("forest cv", forest_cv.mean.rmse, forest_cv.mean.r2);
  //========== End Cell 12 ========

  //========== Cell 13: Tune Candidate Families ========
  // Equivalent in spirit to Grid/Random search over model hyperparameters.
  std::vector<lymph::selection::Candidate> candidates{};
  {
    auto wf = lymph::Workflow{
        .model = lymph::linear_factory::linear_regression(pluss::ai::nodej::LinearFamilyOptions{
            .objective = pluss::ai::nodej::LinearObjective::LeastSquares,
            .regularization = pluss::ai::nodej::RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        }),
    };
    candidates.emplace_back("linear_l2_1e6", std::move(wf));
```

**Expected output**

```text
Candidate configured
```
