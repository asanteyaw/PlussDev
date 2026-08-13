# `search::grid`

## API Signature

- `search::grid(axes: std::vector<GridAxis>, cv: CrossValidationOptions) -> Search`
- `search::grid(axes: std::vector<GridAxis>, split: SplitOptions) -> Search`

## Notes

Instance method on `self`.

## Inputs

- `axes` (`std::vector<GridAxis>`)
  Input feature data used by this call.
- `cv` (`CrossValidationOptions`)
  Method argument; see signature type for accepted form.
- `split` (`SplitOptions`)
  Configuration object controlling algorithm behavior and safety defaults.

## Output

- Returns `Search`.

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
auto value = search::grid(/* axes */, /* cv */);
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
