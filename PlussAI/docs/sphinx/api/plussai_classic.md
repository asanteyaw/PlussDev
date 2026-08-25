# PlussAI `classic`

`classic` is the public namespace for PlussAI's classical machine-learning framework.

- Use `pluss::ai::classic` in public examples and user code.
- Internally, this family is implemented by `pluss::ai::nodej`.
- `classic` owns the low-level tabular/batch ML substrate that `lymph` builds on.

## What Lives Here

- typed batch containers such as `FeatureBatch` and `LabeledBatch`
- split plans and split helpers
- preprocessing and feature transforms
- model families and family options
- builders, flows, metrics, tuning, and persistence helpers
- online and sparse runtime surfaces

## Public Positioning

Teach `classic` as the direct public family for:

- classical supervised learning
- classical unsupervised learning
- feature preprocessing
- metrics and validation primitives
- low-level batch-first ML workflows

Do not teach `nodej` in user-facing prose. That name is internal.

## Include Surface

```cpp
#include <pluss/ai/ml.h>
```

## Public Namespace

```cpp
namespace cls = pluss::ai::classic;
```

## Relationship To Other Families

- `lymph` is the higher-level convenience/model layer over `classic`
- `nlp` reuses `classic` types for text vectorization and feature batches
- `procedure` can persist and route `classic` artifacts

## Documentation Status

The old generated `nodej_*` reference has been retired. This handwritten branch is
the new public entrypoint and will grow method-by-method in the same style as the
PlussMic API reference.
