# PlussAI `lymph`

`lymph` is PlussAI's high-level modeling workspace.

It is not the lowest-level classical-ML substrate. It is the public layer you use
when you want to build, validate, compare, tune, persist, and reuse modeling
workflows without hand-plumbing every batch, split, metric, and persistence step
yourself.

## Public Positioning

Teach `lymph` as the workspace for model-building and evaluation.

Use `classic` when you want the lower-level contracts directly:

- raw batch and split mechanics
- lower-level metric and estimator families
- implementation-facing building blocks

Use `lymph` when you want the higher-level workflow:

- a stable `Model` or `Workflow` object to carry through a project
- reusable validation plans instead of ad-hoc split code
- payload-aware learning and inference
- spotchecking, search, comparison, and persistence

That distinction matters. If users come to `lymph` expecting a bag of disconnected
utilities, they will underuse it. The framework is most valuable when you let it
coordinate a modeling run end to end.

## Include Surface

```cpp
#include <pluss/ai/lymph.h>
```

## Public Namespace

```cpp
namespace lymph = pluss::ai::lymph;
```

## Relationship To Other Frameworks

`lymph` lives above the lower classical-ML layer and beside the broader runtime and
artifact systems.

- `classic` supplies the lower-level estimator, batch, split, and metric machinery.
- `procedure` can persist and orchestrate `lymph` artifacts in larger execution graphs.
- `ice` can coexist with `lymph` in broader AI systems where classical and neural
  workflows live side by side.

The important architectural idea is separation of responsibility:

- `classic` answers "how does the lower-level ML primitive behave?"
- `lymph` answers "how does a user build and run a modeling workflow cleanly?"
- `procedure` answers "how do I persist and replay the workflow as an artifact?"

## Mental Model

If you only remember one idea, remember this:

`lymph` is where intent becomes workflow.

You normally do not stay at the level of raw arrays, raw folds, and raw evaluation
plumbing. You move into a smaller set of durable objects:

- `Model` for trainable pipelines
- `Workflow` for richer deployable orchestration
- `Payload` for explicitly described inputs
- `ValidationOptions` and `validate::Plan` for validation policy
- `Builder`, `Suite`, and `Search` for comparison and tuning

That object model is what keeps large projects from collapsing into one-off helper
functions and fragile experiment scripts.

## Typical Project Flow

A realistic `lymph` flow usually looks like this:

1. choose or construct a `Model`
2. attach prep or transform stages
3. describe the incoming data with `Payload` when needed
4. choose a validation family such as `validate::cross(...)` or `validate::time(...)`
5. run comparison, spotchecking, or search
6. train the surviving workflow
7. persist the resulting artifact

That is the level `lymph` is trying to stabilize.

## Lymph Families

This reference follows the framework -> family -> object narrowing order.

Each family page should answer three things clearly:

- when you enter that family
- which objects you carry on the left-hand side
- which functions or builders are the main entrypoints

```{toctree}
:maxdepth: 3

lymph/metrics
validate <lymph/split>
lymph/prep
lymph/model
lymph/tune
lymph/io
```

## Reading Strategy

If you are learning `lymph` for the first time, this is the most effective order:

1. [model](lymph/model.md) to understand the main durable objects
2. [validate](lymph/split.md) to understand validation planning
3. [metrics](lymph/metrics.md) to understand validation inputs and evaluation outputs
4. [prep](lymph/prep.md) to understand reusable input transformation
5. [tune](lymph/tune.md) to understand comparison and search
6. [io](lymph/io.md) to understand save/load and artifact reuse

That sequence mirrors how real projects usually mature.

## Documentation Standard For This Branch

This handwritten branch is the public `lymph` reference.

The goal is not to dump signatures. The goal is to teach the framework well enough
that a reader can move from "I know the names" to "I know how to use this in a real
project without improvising architecture every time."
