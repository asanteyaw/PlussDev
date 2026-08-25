# `lymph::Model`

## Summary

`lymph::Model` is the main cheap-copy trainable handle in Lymph.

It is the object most users should expect to carry through everyday modeling work.
Factories produce it, validation surfaces score it, tuning surfaces compare it,
and persistence surfaces save and reload it.

## What The Object Represents

`Model` represents one staged modeling pipeline.

That wording matters. It is not only a learner shell. It can accumulate:

- preprocessors
- encoders
- scaling stages
- transforms
- the learner itself
- inference and assessment behavior

That makes it much closer to a practical modeling artifact than a naked estimator.

## Why Users Reach For It

Use `Model` when one trainable staged artifact is enough.

That covers a large amount of real work:

- tabular classification
- tabular regression
- feature engineering plus one learner
- persisted production artifacts that do not need broader orchestration

Move to `Workflow` when the project needs a stronger runtime contract around the
model rather than just the model itself.

## Signature

```cpp
lymph::Model lymph::model::logistic_regression(
    classic::LinearFamilyOptions options = {});

lymph::ModelPrep lymph::Model::prepare();
lymph::Model& lymph::Model::learn_(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::TrainOptions& train_options);
pluss::Series lymph::Model::infer(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options) const;
lymph::MetricReport lymph::Model::assess(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::AssessOptions& assess_options) const;
```

## Practical Example

This page uses the exact verifier program `lymph_model_builder_docverify_demo.cpp`.

### Example 1

This example builds a logistic model, attaches scaling and encoding through
`ModelPrep`, trains it, infers on the same frame, and prints the resulting model
state and metrics.

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

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  model.learn_(raw, data_options, train_options);
  const Series predicted = model.infer(raw, data_options);
  const lymph::MetricReport report = model.assess(
      raw,
      data_options,
      lymph::AssessOptions{
          .metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1},
      });

  std::cout << "== model ==\n";
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
== model ==
stage_count=2 trained=1 task_kind=0
predictions=pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
accuracy=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1] f1=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

Read that output as confirmation of three things:

- the model is carrying two attached prep stages
- the model is now trained state, not a bare factory result
- prediction and assessment both flow through the same staged artifact

## Related Pages

- [Builder](Builder.md)
- [Workflow](Workflow.md)
- [Payload](Payload.md)
- [Policy](Policy.md)
