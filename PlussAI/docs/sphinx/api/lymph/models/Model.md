# `lymph::models::Model`

`Model` is the trainable estimator artifact at the centre of the `models` family:
one algorithm, its attached preparation, and — once trained — its learned
parameters, carried as a single cheap-to-copy value.

> See the [models family](../models.md) for what this concern owns, and
> [Introduction](Model/Introduction.md) for the object's lifecycle and state.

## Scope And Alias

`Model` is the family-owned spelling of the handle. The factories that produce it
live under the model subfamilies (`linear`, `tree`, `ensemble`, …) and are
re-exported at the family root, so both spellings compile:

```cpp
lymph::models::Model m1 = lymph::models::linear::logistic_regression();  // fully explicit
lymph::models::Model m2 = lymph::models::logistic_regression();          // family shortcut
```

Pick the explicit form when the reader benefits from seeing which subfamily an
algorithm belongs to; pick the shortcut for terse downstream code. The method
pages use both.

## Construction

```cpp
// A representative selection -- each returns a lymph::models::Model.
lymph::models::Model lymph::models::logistic_regression(LinearFamilyOptions = {});
lymph::models::Model lymph::models::linear_regression(LinearFamilyOptions = {});
lymph::models::Model lymph::models::linear::logistic_regression(LinearFamilyOptions = {});
lymph::models::Model lymph::models::tree::decision_tree_classifier(TreeFamilyOptions = {});
lymph::models::Model lymph::models::ensemble::random_forest_classifier(EnsembleFamilyOptions = {});
lymph::models::Model lymph::models::neighbor::knn_classifier(NeighborFamilyOptions = {});
// ... and the rest of the linear / tree / ensemble / neighbor / kernel /
//     bayes / cluster / decompose / detect subfamilies.
```

A factory returns an untrained *template*: the algorithm is chosen, nothing is
learned. [`prepare`](Model/prepare.md) / [`append`](Model/append.md) attach
feature work; [`learn`](Model/learn.md) fits it.

## State

`bool trained() const`
: `true` once [`learn`](Model/learn.md) has fitted the estimator.

`std::int64_t stage_count() const`
: The number of resolved, fitted preparation stages — `0` on a template, even one
  with preparation attached, until `learn` runs.

`TaskKind task_kind() const`
: `Classification`, `Regression`, or `Transform` — a property of the chosen
  algorithm, not a training result.

## Methods

| Method | Summary |
|--------|---------|
| [`append`](Model/append.md) | Attach one preparation stage, routed at chosen columns. |
| [`assess`](Model/assess.md) | Run the model on a labelled frame; return a `MetricReport`. |
| [`feature_transformer`](Model/feature_transformer.md) | Attach a pre-built multi-branch `prep::FeatureTransformer` (`function_transformer` for a single whole-frame fn). |
| [`infer`](Model/infer.md) | Predict for new feature rows (`infer_scores` for raw scores). |
| [`learn`](Model/learn.md) | Fit the preparation, then the estimator (`learn_` in place). |
| [`load`](Model/load.md) | Static factory: read a saved model back, fully trained. |
| [`prepare`](Model/prepare.md) | Fluent builder for attaching preparation stages. |
| [`save`](Model/save.md) | Write the trained model to a bundle file. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Model/Introduction
Model/append
Model/assess
Model/feature_transformer
Model/infer
Model/learn
Model/load
Model/prepare
Model/save
```
<!-- apiref:toctree:end -->

## Changelog

Added in 0.1.
