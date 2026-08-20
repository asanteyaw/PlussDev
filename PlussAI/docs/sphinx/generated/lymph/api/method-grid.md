# `grid`

## API Signature

- `grid(axes: std::vector<GridAxis>, cv: metrics::ValidationOptions) -> Search`

## Notes

Namespace-level API function.

## Inputs

- `axes` (`std::vector<GridAxis>`)
  Input feature data used by this call.
- `cv` (`metrics::ValidationOptions`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Search`.

## Options

Option semantics used by this method:

- `ValidationOptions` (`struct`):
  - `CVPlanKind plan = CVPlanKind::CrossVal`
    Option field used by this method.
  - `double holdout_fraction = 0.2`
    Option field used by this method.
  - `std::int64_t folds = 5`
    Controls fold/trial/count sizing.
  - `std::int64_t repeats = 1`
    Option field used by this method.
  - `std::int64_t time_gap = 0`
    Option field used by this method.
  - `std::int64_t time_test_size = 0`
    Option field used by this method.
  - `std::int64_t time_max_train_size = 0`
    Controls fold/trial/count sizing.
  - `bool stratified = false`
    Option field used by this method.
  - `Series groups = Series::undefined()`
    Option field used by this method.
  - `bool reliability_defaults = true`
    Option field used by this method.
  - `bool shuffle = true`
    Option field used by this method.
  - `std::int64_t max_parallel = 0`
    Option field used by this method.
  - `std::uint64_t random_state = 0`
    Random seed for reproducible behavior.
  - `bool deterministic = true`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = grid(/* axes */, /* cv */);
out.info("grid", value);
```

**Expected output**

```text
grid returned Search
```

### Example 2 (Demo Pattern)

```cpp
        }
        if (name.find("knn") != std::string::npos || name.find("neighbor") != std::string::npos) use_neighbor = true;
        if (name.find("svr") != std::string::npos || name.find("kernel") != std::string::npos) use_kernel = true;
        if (name.find("bayes") != std::string::npos) use_bayes = true;
      }
    }

    //========== tuning: targeted search on top-three families ========
    // This section creates a denser candidate grid only around families
    // that appeared in the top-three broad sweep.
    std::vector<lymph::selection::Candidate> tuned_candidates{};

    if (use_linear) {
      auto wf1 = lymph::models::linear_regression(LinearFamilyOptions{          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-9,
          .fit_intercept = true,
      });
      wf1.simple_imputer(lymph::prep::SimpleImputerOptions{.strategy = lymph::prep::SimpleImputerStrategy::Median});
      wf1.standard_scaler();
      tuned_candidates.emplace_back("tuned_linear_l2_1e9", std::move(wf1));
```

**Expected output**

```text
grid returned Search
```
