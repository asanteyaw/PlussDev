# Lymph `prep`

`prep` is the Lymph family for preprocessing, routing, and feature-shaping stages
that sit around a model or workflow.

Use it when you are deciding:

- which numeric columns should be scaled
- which categorical columns should be encoded
- which branches should receive which feature subsets
- how to keep train-time and infer-time preprocessing identical

## What This Family Owns

`prep` owns the data-conditioning layer of Lymph.

That includes:

- routing with `ColumnRouter`
- attachable stage descriptions with `Stage`
- reusable transformer objects such as `StandardScaler`
- categorical expansion with `OneHotEncoder`
- branch-local composite transformation with `FeatureTransformer`

The core design point is simple:

- `model` answers "what learner am I using?"
- `prep` answers "what must happen to the features before learning?"

That separation is important in production ML because many silent failures are not
learner failures. They are preprocessing drift:

- scaling applied during training but forgotten at inference
- categories seen during inference but not handled robustly
- feature subsets routed differently in different experiments
- custom transformations living as loose helper code instead of model contract

## Read This Family In Two Layers

There are two normal ways to use `prep`.

### Layer 1. Standalone transformer objects

Use:

- `prep::StandardScaler`
- `prep::OneHotEncoder`
- `FeatureTransformer`

when you want a reusable transformer object in its own right.

### Layer 2. Attached pipeline stages

Use:

- `stage::standard_scaler(...)`
- `stage::one_hot_encoder(...)`
- `Model::append(...)`
- `ModelPrep`

when you want preprocessing to travel with the model artifact itself.

This second layer is usually the one that matters most in real work, because it
keeps training and inference on the same rails.

## Main Objects

```{toctree}
:maxdepth: 2

prep/ColumnRouter
prep/Stage
prep/StandardScaler
prep/OneHotEncoder
prep/FeatureTransformer
```

## Practical Examples

These examples are verified from real executables under
the lymph documentation verifier examples. The code blocks below are the same programs that
were built and run to produce the shown output.

### Example 1

This example shows the normal high-level path: use `ModelPrep` to attach scaling
and encoding to a model, then train, infer, and assess through that attached prep
contract.

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
      AssessOptions{
          .metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1},
      });

  std::cout << "== prep ==\n";
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
== prep ==
stage_count=2 trained=1 task_kind=0
predictions=pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
accuracy=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1] f1=pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

What this teaches:

- `ModelPrep` is not a side helper; it mutates the model’s attached stage graph
- scaling and encoding become part of the same trained artifact
- assessment happens after the exact same preprocessing contract used for
  training and inference

### Example 2

This example shows the lower-level transformer route. Instead of attaching prep
directly to a model, it builds a reusable `FeatureTransformer` that standardizes a
numeric branch, one-hot encodes a categorical branch, and passes the remainder
through.

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
      {"region", Thread(pluss::make_note({"north", "west", "north", "south", "east", "south", "west", "east"}))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9, 1.8, 2.1, 2.4, 2.7}, {8}, f64))},
  });

  lymph::DataFrameBatchOptions options{};
  options.feature_names = {"income", "region", "balance"};
  options.note_policy = lymph::DataNotePolicy::Factorize;
  options.layout = lymph::ObservationLayout::Rows;

  lymph::prep::FeatureTransformer transformer = lymph::prep::feature_transformer(
      raw,
      options,
      {
          lymph::prep::branch::compose(
              {
                  lymph::stage::standard_scaler(),
              },
              {"income"}),
          lymph::prep::branch::one_hot_encoder(
              lymph::prep::OneHotEncoderOptions{
                  .max_categories = 8,
                  .overflow = lymph::prep::CategoryOverflowPolicy::KeepMostFrequent,
                  .include_unknown_bucket = true,
              },
              {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  const lymph::Payload payload = lymph::payload::make(raw)
                                     .features({"income", "region", "balance"})
                                     .note(lymph::DataNotePolicy::Factorize);
  const lymph::FeatureBatch batch = payload.feature_batch();
  const Series transformed =
      lymph::resolve(transformer.learn_map(batch), "feature_transformer.learn_map");

  std::cout << "== feature_transformer ==\n";
  std::cout << "trained=" << transformer.trained()
            << " stage_count=" << transformer.stage_count()
            << " rows=" << transformed.sizes()[0]
            << " features=" << transformed.sizes()[1] << "\n";
  std::cout << transformed << "\n";

  return 0;
}
```

Output

```text
== feature_transformer ==
trained=1 stage_count=2 rows=8 features=7
pluss::Series(shape=[8,7], strides=[7,1], dtype=float64, platform=cpu, numel=56, contiguous=true)
data=[[-1.3608276, 0, ... , 0.2],
  [-1.0886621, 0, ... , 0.4],
  ...
 [1.3608276, 1, ... , 2.7]]
```

This is the more compositional `prep` shape:

- it is still branch-aware
- it is still trained state, not a loose stateless mapper
- it can be reused independently of any one specific model instance
- branch construction belongs to `lymph::prep::branch::*`
- individual transform descriptors belong to `lymph::stage::*`
- `lymph::workflow::*` stays reserved for workflow composition, not prep routing

## Recommended Reading Order

1. [ColumnRouter](prep/ColumnRouter.md)
2. [Stage](prep/Stage.md)
3. [StandardScaler](prep/StandardScaler.md)
4. [OneHotEncoder](prep/OneHotEncoder.md)
5. [FeatureTransformer](prep/FeatureTransformer.md)
