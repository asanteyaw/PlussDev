# `lymph::Workflow`

## Summary

`lymph::Workflow` is the deployable composition object in Lymph.

Use it when a plain `Model` handle is no longer enough and you want the full
runtime contract to travel together:

- the trainable model
- data-admission rules through `Policy`
- attached preprocessing stages
- one persisted artifact that reflects how the system is meant to behave at
  deployment time

## Why It Exists

Lymph distinguishes between:

- the thing that learns
- the thing that is deployed

In trivial projects those can be treated as the same object. In serious projects
they diverge quickly.

For example:

- the model may train correctly, but inference should reject frames missing a
  required feature or time key
- a production artifact may need stronger runtime guardrails than an experiment
  notebook
- policy and prep should persist with the model rather than live in side scripts

`Workflow` is the answer to that separation.

## Signature

```cpp
lymph::Workflow lymph::workflow::make(
    lymph::Model model,
    lymph::Policy policy);
lymph::Workflow& lymph::Workflow::learn_(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::TrainOptions& train_options);
pluss::Series lymph::Workflow::infer(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options) const;
lymph::MetricReport lymph::Workflow::assess(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::AssessOptions& assess_options) const;
void lymph::Workflow::save(const std::string& path) const;
lymph::Workflow lymph::workflow::load(const std::string& path);
```

## Practical Example

This page uses the exact verifier program
`lymph_workflow_payload_docverify_demo.cpp`.

### Example 1

This example builds a workflow around a staged model and a policy, runs inference
on new data, saves the workflow, reloads it, and confirms prediction parity after
the roundtrip.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

namespace {

std::vector<double> to_host_values(const pluss::Series& values) {
  return values.to(pluss::pCPU, pluss::ScalarType::Float64)
      .reshape({values.numel()})
      .to_vec<double>();
}

bool same_values(const pluss::Series& left, const pluss::Series& right) {
  return to_host_values(left) == to_host_values(right);
}

} // namespace

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classic = pluss::ai::classic;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"customer_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106, 107, 108}, {8}, f64))},
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-02T00:00:00Z",
          "2026-01-03T00:00:00Z",
          "2026-01-04T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-06T00:00:00Z",
          "2026-01-07T00:00:00Z",
          "2026-01-08T00:00:00Z",
      }))},
  });

  classic::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = classic::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  lymph::Model model = lymph::model::logistic_regression(logistic_options);
  lymph::ModelPrep prep = model.prepare();
  prep.standardize({"income", "balance", "late"});
  prep.encode(
      lymph::columnrouter::names({"region"}),
      lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      });

  lymph::Policy policy{};
  policy.drop({"customer_id"});
  policy.require({"income", "balance", "late", "region", "observed_at"});

  lymph::Workflow workflow = lymph::workflow::make(model, policy);

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  workflow.learn_(raw, data_options, train_options);

  const DataFrame incoming({
      {"customer_id", Thread(pluss::make_series({201, 202, 203}, {3}, f64))},
      {"income", Thread(pluss::make_series({1.2, 4.4, 1.6}, {3}, f64))},
      {"balance", Thread(pluss::make_series({0.3, 2.3, 0.5}, {3}, f64))},
      {"late", Thread(pluss::make_series({0, 1, 0}, {3}, f64))},
      {"region", Thread(pluss::make_note({"north", "south", "west"}))},
      {"observed_at", Thread(pluss::make_timeline({
          "2026-02-01T00:00:00Z",
          "2026-02-02T00:00:00Z",
          "2026-02-03T00:00:00Z",
      }))},
  });

  const Series workflow_pred = workflow.infer(incoming, data_options);

  const std::filesystem::path bundle_dir = std::filesystem::temp_directory_path();
  const std::filesystem::path workflow_path =
      bundle_dir / "plussai_lymph_workflow_payload_docverify.bundle";
  workflow.save(workflow_path.string());
  const lymph::Workflow loaded_workflow =
      lymph::workflow::load(workflow_path.string());
  const Series loaded_pred = loaded_workflow.infer(incoming, data_options);

  std::cout << "== workflow ==\n";
  std::cout << "predictions=" << workflow_pred << "\n";
  std::cout << "roundtrip_match=" << same_values(workflow_pred, loaded_pred)
            << "\n";

  std::filesystem::remove(workflow_path);
  std::filesystem::remove(workflow_path.string() + ".policy");

  return 0;
}
```

Output

```text
== workflow ==
predictions=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
roundtrip_match=1
```

That output confirms the core workflow promise:

- inference runs under the workflow contract, not only the raw model
- the persisted workflow roundtrips cleanly
- the loaded artifact reproduces the same predictions

## Related Pages

- [Payload](Payload.md)
- [Model](Model.md)
- [Policy](Policy.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

```
<!-- apiref:toctree:end -->
