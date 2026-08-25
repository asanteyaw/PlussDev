# PlussAI `ice`

`ice` is PlussAI's training/runtime orchestration framework for neural workflows.

It sits above `nets` and packages higher-level concerns such as:

- coaches and training loops
- sources and batch feeding
- outlines and model scaffolds
- save/load behavior for trainable models
- training options, callbacks, and runtime execution control

## Public Positioning

Teach `ice` as the family you reach for when you want to train and operate neural
models, not when you want to define low-level modules themselves.

Use `nets` for module construction.
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

- built primarily on `nets`
- can consume `nlp` artifacts for text-oriented deep workflows
- can be persisted/routed through `procedure`

## Documentation Status

This handwritten branch is the public docs entrypoint for `ice`. Method-level pages
should be added here in the same hand-authored style used by PlussMic.
