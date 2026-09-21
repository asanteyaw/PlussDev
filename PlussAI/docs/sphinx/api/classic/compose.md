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

```cpp
namespace cls = pluss::ai::classic;

cls::StatusOr<cls::Flow> made = cls::make_flow(
    cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear_options});
cls::Flow flow = made.move_value();  // Flow is move-only

flow.add_standard_scaler();                 // a typed convenience stage
flow.learn(batch);                           // fits every stage, then the model
cls::Series pred = flow.infer(x).value();
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
