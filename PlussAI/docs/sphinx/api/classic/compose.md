# `pluss::ai::classic::compose`

`compose` owns [`Flow`](compose/Flow.md) — a routed transform pipeline with an
optional final model, learned and driven as one unit — and the pieces around
it: [`make_flow`](compose/make_flow.md) to build one from a
[`ModelSpec`](families/ModelSpec.md), [`TransformSpec`](compose/TransformSpec.md)
(the declarative record of one pipeline stage), and
[`FlowGraphInfo`](compose/FlowGraphInfo.md) (a read-only snapshot of a `Flow`'s
structure).

Where [`estimators`](estimators.md) gives you one model at a time, `compose`
is for the case where preprocessing and the model need to travel together as
a single deployable unit — fit once, persist once, predict once.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Flow`](compose/Flow.md) | the pipeline itself — add stages, route features, attach a model, `learn` / `infer` / `assess`. Aliased `Workflow`. |
| [`make_flow`](compose/make_flow.md) | build a `Flow` seeded with a final model from a `ModelSpec`. Aliased `make_workflow`. |
| [`TransformSpec`](compose/TransformSpec.md) | the declarative record of one stage: kind, options, feature route, remainder policy |
| [`FlowGraphInfo`](compose/FlowGraphInfo.md) | a read-only snapshot of a `Flow`'s stages and state, from `Flow::graph_info()` |

## How To Call It

Build a flow from a model spec, add a standard scaler, fit the flow on a labeled batch, and predict on the same features. The program prints how many predictions came back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace cls = pluss::ai::classic;

cls::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  return batch;
}

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const cls::LabeledBatch batch = fixture_batch(cfg);
  cls::LinearFamilyOptions linear_options{};
  linear_options.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear_options});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();
  flow.learn(batch);
  const cls::Series pred = flow.infer(batch.features).value();
  std::cout << "predictions = " << pred.numel() << "\n";
  return 0;
}
```

Output

```text
predictions = 8
```

## Related Pages

- [`estimators`](estimators.md) — the classifier/regressor/transformer
  `Flow` can carry as its final model.
- [`prep`](prep.md) — the transform kernels behind every `TransformKind` a
  `Flow` stage can hold.
- [`io`](io.md) — `save` / `load`, the higher-level persistence for a whole
  `Flow`, built on `save_blueprint` / `dump_transform_states` /
  `dump_model_state`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

compose/Introduction
compose/Flow
compose/make_flow
compose/TransformSpec
compose/FlowGraphInfo
```
<!-- apiref:toctree:end -->
