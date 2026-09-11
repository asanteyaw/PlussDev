# `classic::families` Introduction

## What This Family Owns

`families` owns the vocabulary for *describing* an estimator: which of the
nine supported families it belongs to, and that family's construction
options. It owns none of the mechanics of actually building, fitting, or
running one — that is [`estimators`](../estimators.md)'s job.

The nine families, and the concern each covers:

| `ModelFamily` | Options page | Covers |
| --- | --- | --- |
| `Linear` | [`LinearFamilyOptions`](LinearFamilyOptions.md) | linear regression, logistic regression, hinge (SVM-style), robust regression |
| `Tree` | [`TreeFamilyOptions`](TreeFamilyOptions.md) | a single decision tree |
| `Ensemble` | [`EnsembleFamilyOptions`](EnsembleFamilyOptions.md) | bagging, random forest, gradient boosting, AdaBoost |
| `Neighbor` | [`NeighborFamilyOptions`](NeighborFamilyOptions.md) | k-nearest-neighbors |
| `Kernel` | [`KernelFamilyOptions`](KernelFamilyOptions.md) | SVM-family kernel methods, one-class detection |
| `Bayes` | [`BayesFamilyOptions`](BayesFamilyOptions.md) | naive Bayes |
| `Cluster` | [`ClusterFamilyOptions`](ClusterFamilyOptions.md) | k-means |
| `Decompose` | [`DecomposeFamilyOptions`](DecomposeFamilyOptions.md) | PCA |
| `Detect` | [`DetectFamilyOptions`](DetectFamilyOptions.md) | isolation-forest outlier detection |

## Why Describe Before Building

[`ModelSpec`](ModelSpec.md) is plain data — a `{task, family, options}`
triple — precisely so it can be compared, ranked, and swept *before* any
estimator object exists. [`tune::sweep`](../tune/sweep.md) and
[`tune::tune`](../tune/tune.md) take a list (or a sampler producing a stream)
of `ModelSpec` values, cross-validate each, and only the caller's own code
ever needs to build the winner into a real estimator via
[`make_classifier`](../estimators/make_classifier.md) /
[`make_regressor`](../estimators/make_regressor.md). Every candidate that
*loses* the comparison never had to be materialized as a trained object at
all.

## Task, Family, And Options Must Agree

A `ModelSpec` is inert data with no validation built in — three independent
fields that happen to need to agree with each other:

- `.task` must match the `make_*` function used (`make_classifier` needs
  `Classification`, `make_regressor` needs `Regression`).
- `.family` must match whichever `*FamilyOptions` alternative is active in
  `.options`.

Both mismatches are caught, but only when the spec is actually used — see
[`ModelSpec`'s Errors section](ModelSpec.md#errors).

## Reading Order

1. [`ModelSpec`](ModelSpec.md) — the container, and both failure modes.
2. Whichever `*FamilyOptions` page matches the family you are using — each is
   self-contained; you do not need to read all nine before using one.

## Related Pages

- [`estimators`](../estimators.md) — turning a `ModelSpec` into something you
  call `learn` / `infer` / `assess` on.
- [`tune`](../tune.md) — ranking `ModelSpec` candidates.
- [`compose`](../compose.md) — a `ModelSpec` as the final model inside a
  `Flow` pipeline.
