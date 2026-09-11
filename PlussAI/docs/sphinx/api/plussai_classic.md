# PlussAI `classic`

`classic` is PlussAI's classical machine-learning framework: the batch-first,
explicit-contract layer for supervised and unsupervised learning, feature
preprocessing, model selection, and persistence.

Everything here operates on typed batches (`FeatureBatch`, `LabeledBatch`) and
returns a `StatusOr<T>` instead of throwing. You choose an estimator by
describing it (`ModelSpec`), build it (`make_classifier` / `make_regressor` /
`make_transformer` or a `Flow`), and drive it with `learn` / `infer` / `assess`.

## Public Positioning

Reach for `classic` when you want the lower-level contracts directly:

- raw batch and split mechanics
- the estimator families and their options, one struct at a time
- transform kernels you compose and route yourself
- metric, tuning, and persistence primitives with no workflow object in the way

Reach for [`lymph`](plussai_lymph.md) when you want those same capabilities
behind a durable `Model` / `Workflow` object, payload-aware learning, reusable
validation plans, and end-to-end orchestration.

## Include Surface

```cpp
#include <pluss/ai/ml.h>
```

## Public Namespace

```cpp
namespace cls = pluss::ai::classic;
```

Prose and signatures on this reference spell the family out in full
(`classic::Flow`, `classic::prep::StandardScaler`) for clarity; runnable
examples declare the short `cls` alias and call through it (`cls::Flow`,
`cls::prep::StandardScaler`), the same way the codebase does. Sub-namespaces
(`classic::prep`, `classic::online`, `classic::io`) are shown at their full
path where an example needs them.

## Relationship To Other Frameworks

- [`lymph`](plussai_lymph.md) is the higher-level model / workflow layer built
  over `classic`.
- `nlp` reuses `classic` batch and metric types for text feature pipelines.
- `procedure` can persist and route `classic` artifacts inside larger execution
  graphs.

The separation of responsibility:

- `classic` answers "how does this ML primitive behave, exactly?"
- `lymph` answers "how do I build and run a modeling workflow cleanly?"
- `procedure` answers "how do I persist and replay it as an artifact?"

## Families

This reference follows the framework -> family -> object narrowing order. Each
family page states what the concern owns, what it does not, and the first type
or function to reach for.

```{toctree}
:maxdepth: 3

classic/core
classic/data
classic/families
classic/estimators
classic/compose
classic/prep
classic/split
classic/metrics
classic/tune
classic/online
classic/sparse
classic/text
classic/features
classic/inspection
classic/balance
classic/neighbors
classic/io
```

## Reading Strategy

1. [core](classic/core.md) and [data](classic/data.md) — the `Status` contract,
   the shared option bags, and how a table or array becomes a batch.
2. [families](classic/families.md) and [estimators](classic/estimators.md) —
   describe an estimator, build it, train it.
3. [prep](classic/prep.md) and [compose](classic/compose.md) — transform
   kernels, and routing them into a single learned `Flow`.
4. [split](classic/split.md), [metrics](classic/metrics.md), and
   [tune](classic/tune.md) — evidence and model selection.
5. [io](classic/io.md) — persist the trained `Flow`.
6. The remaining families ([online](classic/online.md),
   [sparse](classic/sparse.md), [text](classic/text.md),
   [features](classic/features.md), [inspection](classic/inspection.md),
   [balance](classic/balance.md), [neighbors](classic/neighbors.md)) are
   reached for when a project needs them.

## Documentation Standard

This handwritten branch is the public `classic` reference. Each page carries
full prose on every overload, parameter, shape rule, and failure mode, plus
enough verified end-to-end examples that a reader finishes it with no gap about
another way to make the same call.
