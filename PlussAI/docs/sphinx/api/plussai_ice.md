# PlussAI `ice`

`ice` is PlussAI's training/runtime orchestration framework for neural workflows.

It sits above `nn` and packages higher-level concerns such as:

- coaches and training loops
- sources and batch feeding
- outlines and model scaffolds
- save/load behavior for trainable models
- training options, callbacks, and runtime execution control

## Public Positioning

Teach `ice` as the family you reach for when you want to train and operate neural
models, not when you want to define low-level modules themselves.

Use `nn` for module construction.
Use `ice` for training, assessment, orchestration, and model runtime lifecycle.

## Include Surface

```cpp
#include <pluss/ai/ice.h>
```

## Public Namespace

```cpp
namespace ice = pluss::ai::ice;
```

## Relationship To Other Families

- built primarily on `nn`
- can consume `nlp` artifacts for text-oriented deep workflows
- can be persisted/routed through `procedure`

## Categories

This reference follows the framework -> category -> type narrowing order,
matching [`nn`](plussai_nn.md)'s own convention. Each category page states
what the concern owns, what it does not, and the first type or function to
reach for.

```{toctree}
:maxdepth: 3

ice/platform
ice/options
ice/step
ice/model
ice/outline
ice/optim
ice/no_grad
ice/callback
ice/coach
ice/source
ice/io
ice/data
ice/models_ts
ice/models_nlp
ice/models_rl
```

## Reading Strategy

1. [platform](ice/platform.md) -- device-selection policy (`TrainPlatform`,
   `resolve_platform`), referenced by every later category.
2. [options](ice/options.md) -- `TrainOptions` (public alias for `Recipe`)
   and its five nested policy structs -- the central configuration surface
   every `learn()` call across `ice` takes.
3. [step](ice/step.md) -- the core `Batch`/`StepContext`/`StepResult`
   shapes a training loop's per-step methods pass around.
4. [model](ice/model.md) -- `Model`, the generic escape hatch: define
   parameters plus forward/loss, get learn/infer/assess/save/load for
   free.
5. [outline](ice/outline.md) -- `Outline`, the one-liner way to compose
   existing `nn` layers into a trainable `Model`.
6. [optim](ice/optim.md) / [no_grad](ice/no_grad.md) -- the two escape
   hatches for training loops that don't fit `Model`'s generic shape.
7. [callback](ice/callback.md) / [coach](ice/coach.md) -- the
   extensibility hook system and the model-external orchestration
   surface built on it.
8. [source](ice/source.md) / [io](ice/io.md) -- pluggable batch feeding
   and persistence (both plain parameter dumps and full resumable
   checkpoints).
9. [data](ice/data.md) -- the generic tabular/panel `DataSet` and its
   `TimeSeriesDataSet` specialization, feeding every models_* catalog
   entry's own batch type.
10. [models_ts](ice/models_ts.md), [models_nlp](ice/models_nlp.md),
    [models_rl](ice/models_rl.md) -- the "nice/niche" model catalog: a
    probabilistic autoregressive forecaster and the Temporal Fusion
    Transformer, a text classifier, and a discrete action-value agent.

## Documentation Status

In progress -- this handwritten branch is the public `ice` reference,
built to the same standard as [`nn`](plussai_nn.md): full prose on every
overload, parameter, and failure mode, plus verified end-to-end examples.
`platform` is complete. The remaining fourteen categories are scaffolded
(structure, cross-references, and manifest all in place) and are being
filled in the reading-strategy order above.
