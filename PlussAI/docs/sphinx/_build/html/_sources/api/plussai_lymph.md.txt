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

- a stable `models::Model` or `workflow::Workflow` object to carry through a project
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

- `models::Model` for trainable pipelines
- `workflow::Workflow` for richer deployable orchestration
- `workflow::Payload` for explicitly described inputs
- `selection::CrossValOptions` and `selection::Plan` for validation policy
- `models::Builder`, `selection::Suite`, and `selection::Search` for comparison and tuning

That object model is what keeps large projects from collapsing into one-off helper
functions and fragile experiment scripts.

## Typical Project Flow

A realistic `lymph` flow usually looks like this:

1. choose or construct a `models::Model`
2. attach prep or transform stages
3. describe the incoming data with `workflow::Payload` when needed
4. choose a selection path such as `selection::cross(...)` or `selection::time(...)`
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

The left sidebar should narrow as:

- `lymph`
  - main family
    - subfamily
      - `Introduction`
      - object/type pages
      - option pages

This first rebuilt slice focuses on the `models` family so the navigation
pattern can be verified before the rest of the branch is expanded the same way.

```{toctree}
:maxdepth: 4

lymph/models
lymph/prep
lymph/model
lymph/split
lymph/metrics
lymph/tune
lymph/io
```

## Reading Strategy

If you are learning `lymph` for the first time, this is the most effective order:

1. [models](lymph/models.md) to understand the main trainable artifacts
2. verify the family/subfamily navigation pattern in the sidebar
3. expand the remaining main families on the same pattern

That sequence mirrors how real projects usually mature.

## Documentation Standard For This Branch

This handwritten branch is the public `lymph` reference.

The goal is not to dump signatures. The goal is to teach the framework well enough
that a reader can move from "I know the names" to "I know how to use this in a real
project without improvising architecture every time."
