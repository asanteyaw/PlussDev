# `classic::sparse` Introduction

## What This Family Owns

`sparse` owns a coordinate-format matrix type
([`SparseTripletMatrix`](SparseTripletMatrix.md)), a compressed-row form
([`SparseCsrMatrix`](SparseCsrMatrix.md)) every learner actually computes
on, and three linear estimators that train and infer directly on either
form without ever densifying:
[`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md),
[`SparseLinearRegressor`](SparseLinearRegressor.md), and
[`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md). Reach for
this family instead of the batch `Linear` family (under
[`families`](../families.md)) when your feature matrix is naturally sparse
— text bag-of-words counts, one-hot categorical blocks, or anything else
where most entries are structurally zero — and densifying it into an
ordinary `Series` would waste memory or time neither this family nor its
caller can afford.

Like [`online`](../online/Introduction.md), `sparse` is deliberately
**not** part of the [`estimators`](../estimators.md)/
[`compose`](../compose.md)/[`tune`](../tune.md)/[`io`](../io.md) ecosystem:
there is no `ModelSpec` alternative, no `Flow` attachment, no
[`io::save`](../io/save.md) support. It is a self-contained set of classes
and free functions with their own `learn`/`infer`/`assess`/`dump_state`/
`load_state` methods and their own
[`cross_assess_sparse`](cross_assess_sparse.md)/
[`sweep_sparse`](sweep_sparse.md)/[`tune_sparse`](tune_sparse.md) trio,
paralleling but never sharing code with dense `classic`'s own
[`cross_assess`](../metrics/cross_assess.md)/[`sweep`](../tune/sweep.md)/
[`tune`](../tune/tune.md).

## Two Matrix Forms, One Conversion

[`SparseTripletMatrix`](SparseTripletMatrix.md) (COO: parallel row/column/
value `Series`) is the form you build from raw `(row, column, value)`
triples via [`make_sparse_triplet`](make_sparse_triplet.md).
[`SparseCsrMatrix`](SparseCsrMatrix.md) (CSR: `std::vector`-backed, **not**
`Series`-backed — a deliberate, rare CPU-only exception to this reference's
usual "everything is a `Series`" pattern) is what every estimator actually
solves against. Every `learn`/`infer_scores`/`infer`/`assess` overload that
accepts a `SparseTripletMatrix` converts it to CSR internally via
[`sparse_triplet_to_csr`](make_sparse_triplet.md) before doing any work —
calling that conversion yourself first only matters when you plan to reuse
the same matrix across several calls and want to pay the conversion cost
once rather than once per call.

## Options Are Supplied Per Call, Not At Construction

Unlike every batch estimator under [`estimators`](../estimators.md) (which
takes a fixed `ModelSpec` at construction) or
[`online`](../online/Introduction.md)'s two classes (which take a fixed
`OnlineLinearOptions` at construction), all three sparse estimators take
**no** constructor arguments at all — a fresh
[`LinearFamilyOptions`](../families/LinearFamilyOptions.md) is passed to
each `learn` call instead. Only three of that struct's six fields are
actually read anywhere in this family, and which three differs by class:

| Field | `SparseLinearBinaryClassifier` / `SparseLinearMultiClassifier` | `SparseLinearRegressor` |
| --- | --- | --- |
| `regularization` / `regularization_strength` | read (L2 only meaningfully; any other value behaves as no penalty) | read (same) |
| `objective` | **ignored** — always fits `Logistic` | **ignored** — always fits `LeastSquares`; `RobustLeast` behaves identically |
| `fit_intercept` | **ignored** — a bias term is always included | **ignored** — same |
| `elastic_l1_ratio` / `robust_transition` | never read | never read |

An `objective`/`fit_intercept` you explicitly set and expect to take effect
silently does nothing here — see each estimator's own page (starting with
[`SparseLinearBinaryClassifier`'s Construction section](SparseLinearBinaryClassifier.md#construction))
for the individual proof. This is a genuine, narrower surface than the
batch `Linear` family offers, not a bug: full-batch gradient descent at a
fixed `0.05` learning rate is the only solver this family implements, and
every one of these estimators was built around exactly that one solver.

## No Search Strategy

Dense [`tune`](../tune/tune.md) offers five `SearchKind` values — exhaustive
grid, random subsampling, successive halving, Bayesian search — selected
via `TuneOptions::search`. `sparse` has none of this:
[`tune_sparse`](tune_sparse.md) always evaluates every candidate you pass,
every time, and [`sweep_sparse`](sweep_sparse.md) is presently a literal
alias of it (see [`tune_sparse`'s own note](tune_sparse.md) for why the two
names still both exist). If you need a budgeted or strategy-driven search
over sparse candidates, build it yourself around repeated
[`cross_assess_sparse`](cross_assess_sparse.md) calls — there is no
built-in equivalent to reach for.

## Train And Test Metrics, Together

Every [`SparseFoldReport`](SparseCrossReport.md) inside a
[`SparseCrossReport`](SparseCrossReport.md) carries **both** `metrics`
(scored on that fold's test rows) and `train_metrics` (scored on the same
fold's own training rows) — a direct overfitting signal dense
[`CrossAssessReport`](../metrics/CrossAssessReport.md) does not offer at
all. The cost is one extra `assess` call per fold; there is no way to skip
it and get only the test-side metrics.

## Reading Order

1. [`SparseTripletMatrix`](SparseTripletMatrix.md) /
   [`SparseCsrMatrix`](SparseCsrMatrix.md) — the two matrix forms.
2. [`make_sparse_triplet`](make_sparse_triplet.md) — building and
   converting between them.
3. [`SparseLinearBinaryClassifier`](SparseLinearBinaryClassifier.md) —
   the richest of the three estimators (two label conventions, more
   supported metrics).
4. [`SparseLinearRegressor`](SparseLinearRegressor.md) /
   [`SparseLinearMultiClassifier`](SparseLinearMultiClassifier.md) — the
   other two.
5. [`cross_assess_sparse`](cross_assess_sparse.md) →
   [`sweep_sparse`](sweep_sparse.md) / [`tune_sparse`](tune_sparse.md) —
   cross-validated evaluation and candidate ranking.
6. [`SparseCrossReport`](SparseCrossReport.md) /
   [`SparseTuneReport`](SparseTuneReport.md) — the two result types.

## Related Pages

- [`families`](../families.md) — `LinearFamilyOptions`, the struct every
  `learn` call here takes fresh.
- [`online`](../online/Introduction.md) — the other family outside the
  `estimators`/`compose`/`tune`/`io` ecosystem, for a different reason
  (incremental fitting rather than sparse-matrix support).
- [`metrics`](../metrics.md) — the same metric formulas `assess` computes
  here, without the configurability (`ClassReportOptions`) the full
  `classification_summary` path offers.
