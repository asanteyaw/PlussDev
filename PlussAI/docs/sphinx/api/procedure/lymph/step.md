# `pluss::ai::procedure::step`

The three `lymph` adapter overloads: a `lymph::workflow::Policy`
transform step, a `lymph::models::Model` learn/infer/assess step, and a
`lymph::workflow::Workflow` learn/infer/assess step -- all persistable,
all routing on `RunOptions::mode`.

## Signature

```cpp
pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, lymph::workflow::Policy policy);

pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, lymph::models::Model model, lymph::TrainOptions train_options = {}, lymph::InferOptions infer_options = {}, lymph::AssessOptions assess_options = {});

pluss::ai::Step pluss::ai::procedure::step(std::string name, std::string input_name, std::string output_name, lymph::workflow::Workflow workflow, lymph::TrainOptions train_options = {}, lymph::InferOptions infer_options = {}, lymph::AssessOptions assess_options = {});
```

## What It Does

The `Policy` overload builds a step that applies the policy's transform
to a `pluss::table::DataFrame` or `lymph::workflow::Payload` input and
publishes the result, ignoring `RunOptions::mode` entirely (a `Policy`
has no learned state and only one behavior). The `Model`/`Workflow`
overloads build a step that, depending on
[`RunOptions::mode`](../core/RunOptions.md), calls the wrapped
component's own `learn_`/`assess`/`infer` -- `Learn` and `Assess` accept
a `lymph::LabeledBatch` or a `Payload` carrying one; the default `Infer`
accepts a `lymph::FeatureBatch`, a `Payload`, or a plain `DataFrame`.
Every overload marks its step `persistable`, with
`trained()` reflecting the wrapped component's state at the moment the
step was built.

## Parameters

`std::string name`, `std::string input_name`, `std::string output_name`
: The step's name and its one input/output port, same as every other
  adapter overload.
`lymph::workflow::Policy policy`
: The transform to wrap.
`lymph::models::Model model` / `lymph::workflow::Workflow workflow`
: The component to wrap.
`lymph::TrainOptions train_options`
: Passed through to the component's own `learn_` call. Defaults to
  `TrainOptions{}`.
`lymph::InferOptions infer_options`
: Passed through to the component's own `infer` call. Defaults to
  `InferOptions{}`.
`lymph::AssessOptions assess_options`
: Passed through to the component's own `assess` call. Defaults to
  `AssessOptions{}`.

## Returns

A [`Step`](../core/Step.md) value.

The result produced by that step depends on the adapter and run mode:

| Wrapped component | `Infer` | `Learn` | `Assess` |
| --- | --- | --- | --- |
| `workflow::Policy` | Transformed `DataFrame` or `workflow::Payload` | Same transform | Same transform |
| `models::Model` | Prediction `Series` | Trained `models::Model` | `MetricReport` |
| `workflow::Workflow` | Prediction `Series` | Trained `workflow::Workflow` | `MetricReport` |

For a payload policy call, only the payload's frame is replaced. Its feature,
label, weight, group, and time-role declarations are retained.

## Errors

- Throws if `input_name` or `output_name` is empty.
- At run time, throws if the resolved input artifact's type does not
  match what the active mode expects (for example, a `Policy` step given
  neither a `DataFrame` nor a `Payload`, or a `Model`/`Workflow` step in
  `Learn`/`Assess` mode given an unlabeled input).
- A `Payload` passed to the policy overload must contain a frame.
- Failures from the wrapped component's `learn_`, `infer`, `assess`, policy
  validation, or persistence code propagate normally. A procedure configured
  with `fail_fast=false` converts a run-time failure into a failed
  `ProcedureResult`; construction errors still throw.

## Practical Examples

### Example 1

A `Policy` step drops a column from a `DataFrame`.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"id", Slate(pluss::make_series(std::vector<double>{101.0, 102.0}, {2}, cfg))},
      {"income", Slate(pluss::make_series(std::vector<double>{50.0, 80.0}, {2}, cfg))},
  });

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("clean", "raw", "frame", lymph::workflow::Policy{.drop_columns = {"id"}}),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", raw)});
  std::cout << "columns=" << out.get<DataFrame>("frame").nfeatures() << "\n";
  return 0;
}
```

Output

```text
columns=1
```

### Example 2

A `Model` step learns a `lymph::models::linear_regression()` estimator
from a `LabeledBatch`, in `Learn` mode.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0, 4.0}, {4, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{1.0, 3.0, 5.0, 7.0}, {4}, cfg);
  const lymph::LabeledBatch batch{.features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows}, .y = y};

  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("fit", "train", "model", lymph::models::linear_regression()),
  });
  const lymph::models::Model model =
      proc.run({ai::procedure::input("train", batch)}, learn).get<lymph::models::Model>("model");
  std::cout << "trained=" << model.trained() << "\n";
  return 0;
}
```

Output

```text
trained=1
```

### Example 3

A `Workflow` step learns a `decision_tree_regressor`-backed workflow the
same way -- the only difference from the `Model` overload is that a
`Workflow` additionally owns its own preprocessing policy internally.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 0.0, 2.0, 1.0, 3.0, 1.0}, {4, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 3.0, 4.0}, {4}, cfg);
  const lymph::LabeledBatch batch{.features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows}, .y = y};

  lymph::workflow::Workflow wf = lymph::workflow::make(lymph::models::decision_tree_regressor({.max_depth = 2}));
  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("learn", "train", "workflow", wf),
  });
  const lymph::workflow::Workflow trained =
      proc.run({ai::procedure::input("train", batch)}, learn).get<lymph::workflow::Workflow>("workflow");
  std::cout << "trained=" << trained.trained() << "\n";
  return 0;
}
```

Output

```text
trained=1
```

### Example 4: Learn, Infer, And Assess One Model

The model overload publishes a different artifact type in each mode. This
single program exercises all three branches and requests two assessment
metrics explicitly.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  const auto cfg = pluss::Config{}
      .with_dtype(pluss::ScalarType::Float64)
      .with_platform(pluss::pCPU);
  const auto x = pluss::make_series(
      std::vector<double>{0.0, 1.0, 1.0, 2.0,
                          2.0, 3.0, 3.0, 4.0},
      {4, 2}, cfg);
  const auto y = pluss::make_series(
      std::vector<double>{1.0, 3.0, 5.0, 7.0}, {4}, cfg);
  const lymph::LabeledBatch batch{
      .features = lymph::FeatureBatch{
          .x = x,
          .layout = lymph::ObservationLayout::Rows,
      },
      .y = y,
  };

  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;
  const auto trainer = ai::procedure::make({
      ai::procedure::step(
          "fit", "train", "model",
          lymph::models::linear_regression()),
  });
  const auto model =
      trainer.run({ai::procedure::input("train", batch)}, learn)
          .get<lymph::models::Model>("model");

  const auto scorer = ai::procedure::make({
      ai::procedure::step("score", "features", "pred", model),
  });
  const auto scored = scorer.run({
      ai::procedure::input("features", batch.features),
  });

  lymph::AssessOptions assess_options{};
  assess_options.metrics = {
      lymph::MetricKind::Mse,
      lymph::MetricKind::R2,
  };
  const auto assessor = ai::procedure::make({
      ai::procedure::step(
          "assess", "eval", "report", model,
          lymph::TrainOptions{},
          lymph::InferOptions{},
          assess_options),
  });
  ai::RunOptions assess{};
  assess.mode = ai::ProcedureMode::Assess;
  const auto report =
      assessor.run({ai::procedure::input("eval", batch)}, assess)
          .get<lymph::MetricReport>("report");

  std::cout << "trained=" << model.trained() << '\n';
  std::cout << "predictions="
            << scored.get<pluss::Series>("pred").numel() << '\n';
  std::cout << "report_metrics=" << report.values.numel() << '\n';
}
```

Output:

```text
trained=1
predictions=4
report_metrics=2
```

### Example 5: Preserve Payload Roles While Applying A Policy

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}
      .with_dtype(pluss::ScalarType::Float64)
      .with_platform(pluss::pCPU);
  const DataFrame raw({
      {"id", Slate(pluss::make_series(
          std::vector<double>{101.0, 102.0, 103.0}, {3}, cfg))},
      {"timestamp", Slate(pluss::make_series(
          std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg))},
      {"income", Slate(pluss::make_series(
          std::vector<double>{50.0, 80.0, 35.0}, {3}, cfg))},
      {"bad", Slate(pluss::make_series(
          std::vector<double>{0.0, 0.0, 1.0}, {3}, cfg))},
  });

  const auto sample = lymph::workflow::payload::make(raw)
      .features({"income"})
      .label("bad")
      .time(lymph::workflow::TimeOptions{
          .index = "timestamp",
          .horizon = 1,
      });

  const auto proc = ai::procedure::make({
      ai::procedure::step(
          "gate", "raw", "clean",
          lymph::workflow::Policy{
              .drop_columns = {"id"},
              .require_columns = {"timestamp", "income", "bad"},
              .reject_missing = {"timestamp", "bad"},
          }),
  });

  const auto out = proc.run({ai::procedure::input("raw", sample)});
  const auto& clean =
      out.get<lymph::workflow::Payload>("clean");
  std::cout << "has_time=" << clean.has_time() << '\n';
  std::cout << "time_feature=" << clean.time_feature() << '\n';
  std::cout << "clean_features=" << clean.frame().nfeatures() << '\n';
}
```

Output:

```text
has_time=1
time_feature=timestamp
clean_features=3
```

### Example 6: Inspect A Mode-Specific Type Error

Inference for a model requires features, a payload, or a frame. A text
artifact is not converted implicitly.

```cpp
#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <string>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;

  ai::ProcedureOptions options{};
  options.fail_fast = false;
  const auto proc = ai::procedure::make({
      ai::procedure::step(
          "score", "features", "pred",
          lymph::models::linear_regression()),
  }, {}, options);

  const auto out = proc.run({
      ai::procedure::input("features", std::string("not features")),
  });
  std::cout << "ok=" << out.ok() << '\n';
  std::cout << "failed_step=" << out.failed_step() << '\n';
  std::cout << "warnings=" << out.warnings().size() << '\n';
}
```

Output:

```text
ok=0
failed_step=score
warnings=1
```

## See Also

- [`workflow::Policy`](../../lymph/workflow/Policy.md) /
  [`models::Model`](../../lymph/models/Model.md) /
  [`workflow::Workflow`](../../lymph/workflow/Workflow.md) -- the
  wrapped types themselves.
- [`ice`](../ice.md) / [`nlp`](../nlp.md) -- the sibling adapters for
  other engines.
