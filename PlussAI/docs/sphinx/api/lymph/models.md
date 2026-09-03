# Lymph `models`

`models` is the Lymph family for trainable modeling artifacts.

This is the branch you enter when the main question is not yet "how do I
deploy the workflow?" or "how do I validate the experiment?" but rather:

- what exact estimator object am I fitting
- how do I attach preparation to that estimator cleanly
- how do I hold several candidate estimators before comparison

That ownership boundary matters because Lymph is intentionally split by
concern:

- `models` owns trainable estimator handles and candidate-model workspaces
- `options` owns the shared configuration bags those objects consume
- `reports` owns the richer result objects those workflows emit
- `prep` owns reusable transformation objects, stages, routes, and feature
  remapping contracts
- `selection` owns evidence policy: cross-validation, holdout, time-aware
  validation, spotchecking, and search
- `workflow` owns the broader deployed orchestration layer

If the object on the left-hand side is "the model I will fit" or "the stable
workspace of candidate models I will compare", you are in `models`.

## What This Family Owns

The family currently revolves around two public objects:

- `lymph::models::Model`: one trainable artifact
- `lymph::models::Builder`: one named candidate-model workspace

Those two objects solve different scales of work.

`Model` is the right object when one estimator is enough and you want to:

- construct it from a family factory such as
  `lymph::models::linear::logistic_regression(...)`
- attach preparation directly to it
- fit, infer, assess, save, and restore the artifact

`Builder` is the right object when one estimator is no longer enough and you
need to:

- register several candidates under stable names
- inspect the candidate set before any comparison begins
- run spotchecking or benchmarking without hand-plumbing candidate vectors
- ask for the remembered best or worst fitted result after comparison

## What This Family Does Not Own

`models` does not decide whether your evaluation policy is statistically
appropriate. That belongs to `selection`.

It also does not own broad deployment metadata or runtime routing. That
belongs to `workflow`.

This separation is what keeps the estimator surface small enough to remain
predictable. A `Model` should feel like a fit-ready artifact, not a dumping
ground for the whole framework.

## How To Read This Family

The most reliable reading order is:

1. [Model](models/Model.md)
2. [builder Introduction](models/builder/Introduction.md)
3. [Builder](models/builder/Builder.md)

Read `Model` first if your project already has a likely champion estimator in
mind. Read `Builder` next if your project still needs a disciplined candidate
workspace before validation or search.

## Subfamilies

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 4
:hidden:

models/Model
models/builder
```
<!-- apiref:toctree:end -->
