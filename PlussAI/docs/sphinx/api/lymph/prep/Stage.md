# `lymph::Stage`

## Summary

`Stage` is the attachable preprocessing descriptor used inside models and
workflows.

Use it when you want preprocessing to live inside the trained artifact rather than
beside it.

## What Stage Answers

`Stage` answers:

"what transformation should this routed subset undergo?"

It does not answer where the subset comes from. That is `ColumnRouter`’s job.

This split matters:

- `Stage` describes the transformation kind
- `ColumnRouter` describes the feature subset

Keeping those roles separate makes pipelines easier to read, audit, and reuse.

## Normal Builders

Construction lives under `lymph::stage::...`.

Common examples:

- `lymph::stage::standard_scaler(...)`
- `lymph::stage::one_hot_encoder(...)`
- `lymph::stage::simple_imputer(...)`
- `lymph::stage::custom(...)`

These builders return attachable stage descriptors. They do not execute on their
own. Execution happens after the stage is attached to a model or workflow and the
artifact is trained or mapped.

## Signature

```cpp
lymph::Stage lymph::stage::standard_scaler(...);
lymph::Stage lymph::stage::one_hot_encoder(...);
lymph::Model& lymph::Model::append(
    lymph::Stage stage,
    lymph::ColumnRouter router);
```

## Practical Example

This page uses the exact verifier program `lymph_prep_stage_docverify_demo.cpp`.

### Example 1

This example attaches two stages explicitly, trains the resulting model, and then
prints both the stage-state summary and the predictive result.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

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
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2, 3.8, 4.2, 4.6, 5.0}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  classic::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = classic::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  lymph::Model model = lymph::model::logistic_regression(logistic_options);
  model.append(
      lymph::stage::standard_scaler(),
      lymph::columnrouter::names({"income", "balance", "late"}));
  model.append(
      lymph::stage::one_hot_encoder(lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      }),
      lymph::columnrouter::names({"region"}));

  model.learn_(raw, data_options, train_options);
  const Series predicted = model.infer(raw, data_options);
  const lymph::MetricReport report = model.assess(
      raw,
      data_options,
      lymph::AssessOptions{
          .metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1},
      });

  std::cout << "== stage ==\n";
  std::cout << "stage_count=" << model.stage_count()
            << " trained=" << model.trained()
            << " task_kind=" << static_cast<int>(model.task_kind()) << "\n";
  std::cout << "predictions=" << predicted << "\n";
  std::cout << "accuracy=" << report.accuracy
            << " f1=" << report.f1 << "\n";

  return 0;
}
```

Output

```text
== stage ==
stage_count=2 trained=1 task_kind=0
predictions=pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
accuracy=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1] f1=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

This is the important lesson:

- stages are not decorative metadata
- they participate in training
- they persist as part of the model’s internal stage graph
- predictive results are produced only after those attached transformations run

## Related Pages

- [ColumnRouter](ColumnRouter.md)
- [StandardScaler](StandardScaler.md)
- [OneHotEncoder](OneHotEncoder.md)
