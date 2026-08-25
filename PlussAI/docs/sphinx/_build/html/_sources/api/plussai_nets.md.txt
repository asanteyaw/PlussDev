# PlussAI `nets`

`nets` is the public namespace for PlussAI's neural-module framework.

- Use `pluss::ai::nets` in public examples and user code.
- Internally, this family is backed by the `NodeK` implementation and the existing
  `pluss::ai::nn` surface.

## What Lives Here

- `Module` and module composition surfaces
- activations, containers, embeddings, losses
- recurrent, attention, transformer, and convolution modules
- neural optimization helpers
- lower-level neural building blocks reused by `ice`

## Public Positioning

Teach `nets` as the direct public family for:

- module-level neural network construction
- reusable trainable blocks
- deep-learning model internals
- neural components used by `ice` models and coaches

Avoid teaching `NodeK` as the public family name. `NodeK` remains an internal
implementation identity.

## Include Surface

```cpp
#include <pluss/ai/nn.h>
```

## Public Namespace

```cpp
namespace net = pluss::ai::nets;
```

## Relationship To Other Families

- `ice` is the higher-level training/runtime layer over `nets`
- `procedure` can route `nets` and `ice` artifacts through staged execution

## Documentation Status

This handwritten branch is now the public docs entrypoint for the neural family.
Detailed method pages should be added here, not generated from internal headers.
