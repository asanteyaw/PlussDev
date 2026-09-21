# `pluss::ai::classic::EstimatorBase`

`EstimatorBase` is the shared contract behind every classifier and regressor
in `classic` — [`ClassifierBase`](ClassifierBase.md) and
[`RegressorBase`](RegressorBase.md) both derive from it, and every concrete
family (`LinearClassifier`, `TreeClassifier`, `KernelClassifier`, …)
implements it. You never construct an `EstimatorBase` directly — you obtain a
`ClassifierBase` or `RegressorBase` from [`make_classifier`](make_classifier.md) /
[`make_regressor`](make_regressor.md) and use it through this base's methods
(plus, for a classifier, `infer_scores`).

## Signature

Every method below is virtual; a concrete family (`LinearClassifier`,
`TreeClassifier`, …) provides the implementation `make_classifier` /
`make_regressor` returns behind the base pointer. `task_kind`, `trained`,
`learn`, `infer`, and `assess` have no default — every family must implement
them. `dump_state` / `load_state` default to a `Status::not_implemented`
result; a family overrides them to support persistence.

```cpp
virtual classic::TaskKind classic::EstimatorBase::task_kind() const noexcept = 0;
virtual bool classic::EstimatorBase::trained() const noexcept = 0;
virtual classic::Status classic::EstimatorBase::learn(
    const classic::LabeledBatch&, const classic::TrainOptions& = {}) = 0;
virtual classic::StatusOr<pluss::Series> classic::EstimatorBase::infer(
    const classic::FeatureBatch&, const classic::InferOptions& = {}) const = 0;
virtual classic::StatusOr<pluss::Series> classic::EstimatorBase::assess(
    const classic::LabeledBatch&, const classic::AssessOptions& = {}) const = 0;

virtual classic::StatusOr<std::vector<pluss::Series>> classic::EstimatorBase::dump_state() const;
virtual classic::Status classic::EstimatorBase::load_state(/* a range of pluss::Series */);
```

## Construction

Never constructed directly (it is abstract — `task_kind`, `trained`, `learn`,
`infer`, and `assess` are pure virtual). Obtain a
[`ClassifierBase`](ClassifierBase.md) or [`RegressorBase`](RegressorBase.md) —
both public subclasses — from [`make_classifier`](make_classifier.md) /
[`make_regressor`](make_regressor.md), and use it through this page's methods.

## State

`task_kind()` and `trained()` are read-only queries over the estimator's
current state — see their own pages under `## Methods`.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `learn(batch, options = {})` | `Status` | [`learn`](EstimatorBase/learn.md) |
| `infer(batch, options = {})` | `StatusOr<Series>` | [`infer`](EstimatorBase/infer.md) |
| `assess(batch, options = {})` | `StatusOr<Series>` | [`assess`](EstimatorBase/assess.md) |
| `task_kind()` | `TaskKind` | [`task_kind`](EstimatorBase/task_kind.md) |
| `trained()` | `bool` | [`trained`](EstimatorBase/trained.md) |
| `dump_state()` | `StatusOr<std::vector<Series>>` | [`dump_state`](EstimatorBase/dump_state.md) |
| `load_state(state)` | `Status` | [`load_state`](EstimatorBase/load_state.md) |

`infer_scores` — the one method `ClassifierBase` adds beyond this contract —
is documented on [`ClassifierBase`](ClassifierBase.md), not here.

## Changelog

- **0.1** — the seven-method estimator contract: fit, predict, score,
  introspect, persist.

## See Also

- [`ClassifierBase`](ClassifierBase.md) / [`RegressorBase`](RegressorBase.md) —
  the two public subclasses you actually hold.
- [`make_classifier`](make_classifier.md) / [`make_regressor`](make_regressor.md) —
  how you obtain one.
- [`TransformerBase`](TransformerBase.md) — the parallel contract for
  transform stages.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

EstimatorBase/Introduction
EstimatorBase/assess
EstimatorBase/dump_state
EstimatorBase/infer
EstimatorBase/learn
EstimatorBase/load_state
EstimatorBase/task_kind
EstimatorBase/trained
```
<!-- apiref:toctree:end -->
