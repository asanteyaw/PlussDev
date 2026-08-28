# `lymph::models::Model`

## Summary

`lymph::models::Model` is the main trainable estimator handle in Lymph.

Factories return it, `prep` objects can be attached to it, `selection`
surfaces can score it, and `workflow` can later wrap it into a broader
orchestration contract.

The important design point is that `Model` is cheap to copy at the call site
but still behaves like a real stateful artifact once training begins. That is
why the object supports both:

- mutating training via `learn_(...)`
- non-mutating training via `learn(...) const`

The mutating path is the direct "fit this object" route. The non-mutating path
is the safer "keep the original template untouched and return a trained copy"
route.

## Signature

```cpp
lymph::models::Model lymph::models::linear::logistic_regression(
    lymph::LinearFamilyOptions options = {});

lymph::models::Model& lymph::models::Model::append(
    lymph::prep::Stage stage,
    lymph::prep::Route route);

lymph::models::ModelPrep lymph::models::Model::prepare() noexcept;

lymph::models::Model& lymph::models::Model::learn_(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::TrainOptions& train_options = {});

lymph::models::Model lymph::models::Model::learn(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::TrainOptions& train_options = {}) const;

pluss::Series lymph::models::Model::infer(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options) const;

lymph::MetricReport lymph::models::Model::assess(
    const pluss::table::DataFrame& frame,
    const lymph::DataFrameBatchOptions& data_options,
    const lymph::AssessOptions& assess_options = {}) const;

void lymph::models::Model::save(const std::string& path) const;

lymph::models::Model lymph::models::Model::load(const std::string& path);
```

## Why This Object Exists

`Model` is the object that keeps everyday estimator work from collapsing into
raw family options, one-off helper code, and ad-hoc state threading.

You use it when one staged, trainable artifact is enough:

- tabular classification
- tabular regression
- transform-style models
- persisted classical-ML artifacts

Move upward to `workflow::Workflow` when the project needs stronger runtime
orchestration than the trainable artifact alone.

## Construction Paths

There are two normal ways to shape a `Model` before fitting:

- direct family construction from a model factory
- staged extension after construction

Direct family construction answers the question "which estimator family do I
want?" Examples include:

- `lymph::models::linear::logistic_regression(...)`
- `lymph::models::tree::decision_tree_classifier(...)`
- `lymph::models::ensemble::random_forest_classifier(...)`

After that, you can shape the artifact in one of two preparation styles:

- `prepare()` for fluent model-bound preparation planning
- `append(stage, route)` for explicit stage-plus-route assembly

Those two styles solve the same broad problem but serve different readers:

- `prepare()` is usually the cleaner public path for everyday work
- `append(...)` is better when stage objects and route objects already exist
  independently and you want to attach them explicitly

## State Semantics

Three state ideas matter more than anything else on this surface:

- `trained()`
- `stage_count()`
- the difference between pending and materialized preparation

Pending preparation is important. When you call `prepare().standardize(...)`
or `append(...)`, the route may depend on the real input frame. Lymph therefore
lets those stages remain pending until `learn_()` or `learn()` sees the real
data and resolves them.

That is why `stage_count()` can remain `0` after preparation has been planned.
Once learning happens on a real frame, the planned stages materialize and the
count becomes the number of attached, resolved stages.

## Learning, Inference, and Assessment

The normal lifecycle is:

1. construct the model
2. optionally attach preparation
3. choose how a frame becomes `(x, y)` via `DataFrameBatchOptions`
4. train with `learn_()` or `learn()`
5. call `infer()` for predictions
6. call `assess()` for metric reports

Use `learn_()` when the left-hand object is meant to become the trained
artifact.

Use `learn()` when the current object is only a reusable template and you want
the training result to come back as a separate `Model`.

## Practical Examples

This page uses the exact verifier programs:

- `lymph_models_model_docverify_demo.cpp`
- `lymph_models_model_append_docverify_demo.cpp`

### Example 1

This example shows the full estimator lifecycle: construction, pending
preparation, materialization on learning, inference, assessment, and the
non-mutating `learn(...)` sibling.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
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

  lymph::LinearFamilyOptions logistic_options{};
  logistic_options.regularization = lymph::RegularizationKind::L2;
  logistic_options.regularization_strength = 1e-3;
  logistic_options.fit_intercept = true;

  lymph::models::Model model = lymph::models::linear::logistic_regression(logistic_options);
  std::cout << "== initial_model ==\n";
  std::cout << "trained=" << std::boolalpha << model.trained()
            << " stage_count=" << model.stage_count()
            << " task_kind=" << static_cast<int>(model.task_kind()) << "\n";

  lymph::models::ModelPrep prep = model.prepare();
  const lymph::prep::Route categorical = prep.names({"region"});
  prep.standardize(prep.remaining(categorical));
  prep.encode(
      categorical,
      lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      });
  std::cout << "== after_prepare ==\n";
  std::cout << "trained=" << model.trained()
            << " stage_count=" << model.stage_count() << "\n";

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"income", "balance", "late", "region"};
  data_options.label_feature = "default";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  lymph::TrainOptions train_options{};
  train_options.max_iterations = 600;
  train_options.tolerance = 1e-8;
  train_options.random = {.seed = 20260825ULL, .deterministic = true};

  model.learn_(raw, data_options, train_options);
  const Series predicted = model.infer(raw, data_options);
  const lymph::MetricReport report = model.assess(
      raw,
      data_options,
      lymph::AssessOptions{
          .metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1},
      });

  std::cout << "== learned_model ==\n";
  std::cout << "trained=" << model.trained()
            << " stage_count=" << model.stage_count() << "\n";
  std::cout << "predictions=" << predicted << "\n";
  std::cout << "accuracy=" << report.accuracy
            << " f1=" << report.f1 << "\n";

  lymph::models::Model template_model = lymph::models::linear::logistic_regression(logistic_options);
  lymph::models::ModelPrep template_prep = template_model.prepare();
  const lymph::prep::Route template_categorical = template_prep.names({"region"});
  template_prep.standardize(template_prep.remaining(template_categorical));
  template_prep.encode(
      template_categorical,
      lymph::prep::OneHotEncoderOptions{
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      });
  const lymph::models::Model learned_copy = template_model.learn(raw, data_options, train_options);

  std::cout << "== learned_copy ==\n";
  std::cout << "original_trained=" << template_model.trained()
            << " original_stage_count=" << template_model.stage_count()
            << " copy_trained=" << learned_copy.trained()
            << " copy_stage_count=" << learned_copy.stage_count() << "\n";

  return 0;
}
```

Output

```text
== initial_model ==
trained=false stage_count=0 task_kind=0
== after_prepare ==
trained=false stage_count=0
== learned_model ==
trained=true stage_count=2
predictions=pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
accuracy=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1] f1=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
== learned_copy ==
original_trained=false original_stage_count=0 copy_trained=true copy_stage_count=2
```

A few details in that output are worth reading carefully:

- `stage_count=0` after `prepare()` confirms that preparation can remain
  pending until the frame is seen
- `stage_count=2` after learning confirms that both planned stages
  materialized on the real frame
- `learn(...)` left the template untouched while returning a trained copy

### Example 2

This example uses the explicit `append(stage, route)` path rather than the
fluent `prepare()` accessor. It is the better fit when stage objects and route
objects already exist independently and you want to assemble the model
deliberately.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"x1", Thread(pluss::make_series({1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64))},
      {"x2", Thread(pluss::make_series({1000, 2000, 500, 4000, 1500, 3000, 800, 3500}, {8}, f64))},
      {"category", Thread(pluss::make_note({"a", "b", "a", "b", "a", "b", "a", "b"}))},
      {"y", Thread(pluss::make_series({0, 0, 0, 1, 0, 1, 0, 1}, {8}, f64))},
  });

  const lymph::prep::Route categorical = lymph::prep::route::names({"category"});
  const lymph::prep::Route numeric = lymph::prep::route::remaining_of(categorical);

  lymph::models::Model model = lymph::models::tree::decision_tree_classifier({.max_depth = 3});
  model.append(lymph::prep::stage::standard_scaler(), numeric);
  model.append(
      lymph::prep::stage::one_hot_encoder({
          .max_categories = 8,
          .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
          .include_unknown_bucket = true,
      }),
      categorical);

  std::cout << "== append_before_learn ==\n";
  std::cout << "trained=" << std::boolalpha << model.trained()
            << " stage_count=" << model.stage_count() << "\n";

  lymph::DataFrameBatchOptions data_options{};
  data_options.label_feature = "y";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  model.learn_(raw, data_options);
  const Series predicted = model.infer(raw.drop({"y"}), data_options);

  std::cout << "== append_after_learn ==\n";
  std::cout << "trained=" << model.trained()
            << " stage_count=" << model.stage_count() << "\n";
  std::cout << "predictions=" << predicted << "\n";

  return 0;
}
```

Output

```text
== append_before_learn ==
trained=false stage_count=0
== append_after_learn ==
trained=true stage_count=2
predictions=pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 1, 0, 1, 0, 1]
```

The practical difference from Example 1 is not model capability but assembly
style. Both routes let you defer materialization until real learning happens.
The choice is mostly about whether your project reads better in fluent
model-bound preparation form or in explicit stage-plus-route form.

## Related Pages

- [models family](../models.md)
- [builder Introduction](builder/Introduction.md)
- [Builder](builder/Builder.md)
