# `procedure::lymph` Introduction

## What This Category Owns

`lymph` owns the three ways to wrap a real `lymph` component as a
[`Step`](../core/Step.md): [`workflow::Policy`](../../lymph/workflow/Policy.md)
(a pure preprocessing transform), [`models::Model`](../../lymph/models/Model.md)
(a single trainable estimator), and [`workflow::Workflow`](../../lymph/workflow/Workflow.md)
(a policy plus a model, learned and run together) -- all three built
through the same overloaded [`step`](step.md) free function, all three
persistable and routed on [`RunOptions::mode`](../core/RunOptions.md).

## Why Three Overloads, Not One

A `lymph::workflow::Policy` has no learned state at all -- it only ever
transforms, so its step needs no `RunOptions::mode` branching and no
trained-state check. `lymph::models::Model` and `lymph::workflow::Workflow`
both learn, infer, and assess, but a `Model` is one estimator while a
`Workflow` additionally owns its own preprocessing `Policy` -- wrapping
either preserves that same shape rather than collapsing both into a
lowest-common-denominator interface. Each overload accepts the
component's own `TrainOptions`/`InferOptions`/`AssessOptions` directly,
so a step built this way behaves identically to calling the wrapped
component's own methods by hand.

## Input And Output Matrix

The active mode determines both the accepted artifact types and the value the
step publishes. The policy overload is mode-independent.

| Adapter | Mode | Accepted input | Published output |
| --- | --- | --- | --- |
| `workflow::Policy` | Any | `pluss::table::DataFrame` | Transformed `DataFrame` |
| `workflow::Policy` | Any | `workflow::Payload` containing a frame | A copy of the `Payload` with only its frame replaced; feature, label, and time roles survive |
| `models::Model` | `Learn` | `LabeledBatch`, or labeled `workflow::Payload` | Trained `models::Model` |
| `models::Model` | `Infer` | `FeatureBatch`, `workflow::Payload`, or `DataFrame` | Prediction `Series` |
| `models::Model` | `Assess` | `LabeledBatch`, or labeled `workflow::Payload` | `MetricReport` |
| `workflow::Workflow` | `Learn` | `LabeledBatch`, or labeled `workflow::Payload` | Trained `workflow::Workflow` |
| `workflow::Workflow` | `Infer` | `FeatureBatch`, `workflow::Payload`, or `DataFrame` | Prediction `Series` |
| `workflow::Workflow` | `Assess` | `LabeledBatch`, or labeled `workflow::Payload` | `MetricReport` |

Type checking occurs when the step runs because the same graph may be invoked
in different modes. Supplying an unlabeled `FeatureBatch` in `Learn` mode, for
example, is not interpreted as inference and is not silently accepted; the
step rejects it with a mode-specific diagnostic.

## Ownership And Learned State

Each overload moves its policy, model, or workflow into shared state owned by
the step. The caller may therefore destroy the original component after
constructing the procedure. Calls to `Learn` mutate that owned state, and a
later `Infer` call through the same `Procedure` observes the learned state.
The artifact returned by `Learn` is a value snapshot of the now-trained model
or workflow, useful when the trained component should leave the graph.

`Step::trained()` is metadata captured when the step is constructed; it does
not become a live view of later learning. Test the component returned by the
`Learn` result when you need the post-run state.

## Payload Preservation

A policy step receiving a `workflow::Payload` transforms the payload's frame
and returns `payload.with_frame(...)`. It does not discard the declared feature
set, label, weight, group, or time role. This makes policy admission suitable
for the front of a larger graph: downstream model steps still receive the
semantic roles assigned before the policy ran. A payload without a frame is
rejected because there is nothing for a tabular policy to transform.

## Persistence

All three adapters are persistable and reloadable. A bundle records a fixed
component key, the adapter schema, the wrapped component schema, and the
training/inference/assessment options needed to reconstruct equivalent step
behavior. Series-valued option fields are written as sidecars rather than
flattened into textual metadata. `Procedure::load()` validates checksums and
the reconstructed contract before returning the graph.

## Example: Preserve Payload Roles Through A Policy

```cpp
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

const auto result = proc.run({ai::procedure::input("raw", sample)});
const auto& clean = result.get<lymph::workflow::Payload>("clean");

std::cout << "has_time=" << clean.has_time() << '\n';
std::cout << "time_feature=" << clean.time_feature() << '\n';
std::cout << "clean_features=" << clean.frame().nfeatures() << '\n';
```

Output:

```text
has_time=1
time_feature=timestamp
clean_features=3
```

The policy removes `id`, but the payload still knows that `timestamp` is its
time feature and that `bad` is its label.

## Example: Learn Once, Then Infer

```cpp
auto workflow = lymph::workflow::make(
    lymph::models::decision_tree_regressor({.max_depth = 3}));

ai::RunOptions learn{};
learn.mode = ai::ProcedureMode::Learn;

const auto trainer = ai::procedure::make({
    ai::procedure::step("learn", "train", "workflow", workflow),
});
const auto trained =
    trainer.run({ai::procedure::input("train", batch)}, learn)
        .get<lymph::workflow::Workflow>("workflow");

const auto scorer = ai::procedure::make({
    ai::procedure::step("infer", "features", "pred", trained),
});
const auto result = scorer.run({
    ai::procedure::input("features", batch.features),
});

std::cout << "trained=" << trained.trained() << '\n';
std::cout << "predictions="
          << result.get<pluss::Series>("pred").numel() << '\n';
```

Output:

```text
trained=1
predictions=6
```

## Reading Order

1. [`step`](step.md) -- all three overloads, one page.

## Related Pages

- [`build`](../build.md) -- the generic `step` overloads this family
  layers on top of.
- [`core`](../core.md) -- `Step`/`StepOptions`/`StepPersistSpec`, what
  every overload here ultimately builds.
