# `pluss::ai::classic::sparse`

`sparse` owns a coordinate-format matrix
([`SparseTripletMatrix`](sparse/SparseTripletMatrix.md)), the
compressed-row form every learner actually computes on
([`SparseCsrMatrix`](sparse/SparseCsrMatrix.md)), and three linear
estimators — [`SparseLinearBinaryClassifier`](sparse/SparseLinearBinaryClassifier.md),
[`SparseLinearRegressor`](sparse/SparseLinearRegressor.md),
[`SparseLinearMultiClassifier`](sparse/SparseLinearMultiClassifier.md) —
that train and infer directly on either form without ever densifying. Like
[`online`](online.md), it is a small, self-contained module: it does not
participate in [`estimators`](estimators.md)/[`compose`](compose.md)/
[`tune`](tune.md)/[`io`](io.md) the way every other family here does; see
[`Introduction`](sparse/Introduction.md) for exactly what that means, plus
the family's own honest limitations (no search-strategy concept, several
`LinearFamilyOptions` fields silently ignored).

## The Surface

| Type / function | What it gives you | Reach for it when |
| --- | --- | --- |
| [`SparseTripletMatrix`](sparse/SparseTripletMatrix.md) / [`make_sparse_triplet`](sparse/make_sparse_triplet.md) | a validated coordinate-format sparse matrix | building a matrix from raw `(row, column, value)` triples |
| [`SparseCsrMatrix`](sparse/SparseCsrMatrix.md) / [`sparse_triplet_to_csr`](sparse/make_sparse_triplet.md) | the compressed-row form the learners solve against | reusing the same matrix across several calls without repeating the conversion |
| [`SparseLinearBinaryClassifier`](sparse/SparseLinearBinaryClassifier.md) | a binary logistic classifier, fit by full-batch gradient descent | your labels are two-valued (or fold cleanly to two extremes) |
| [`SparseLinearRegressor`](sparse/SparseLinearRegressor.md) | a least-squares regressor, same solver | a continuous target |
| [`SparseLinearMultiClassifier`](sparse/SparseLinearMultiClassifier.md) | one-vs-rest multiclass on top of the binary classifier's own solver | three or more classes |
| [`cross_assess_sparse`](sparse/cross_assess_sparse.md) | per-fold train **and** test metrics for one `LinearFamilyOptions` | judging one configuration's reliability across folds |
| [`sweep_sparse`](sparse/sweep_sparse.md) / [`tune_sparse`](sparse/tune_sparse.md) | rank several `LinearFamilyOptions` candidates (presently identical to each other) | choosing a `regularization_strength`, or any other candidate set, by cross-validated score |
| [`SparseCrossReport`](sparse/SparseCrossReport.md) / [`SparseTuneReport`](sparse/SparseTuneReport.md) | the two result types above | reading fold-by-fold or trial-by-trial detail back out |

## How To Call It

```cpp
namespace cls = pluss::ai::classic;

const cls::SparseTripletMatrix x =
    cls::make_sparse_triplet(rows, cols, row_indices, column_indices, values).value();

cls::SparseLinearBinaryClassifier model;
model.learn(x, y);  // a fresh LinearFamilyOptions{} is implied here
const cls::StatusOr<pluss::Series> predicted = model.infer(x);
```

## Related Pages

- [`families`](families.md) — `LinearFamilyOptions`, passed fresh to every
  `learn` call in this family.
- [`online`](online.md) — the other family outside the
  `estimators`/`compose`/`tune`/`io` ecosystem.
- [`metrics`](metrics.md) — the same metric formulas `assess` computes
  here.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

sparse/Introduction
sparse/SparseTripletMatrix
sparse/SparseCsrMatrix
sparse/make_sparse_triplet
sparse/SparseLinearBinaryClassifier
sparse/SparseLinearRegressor
sparse/SparseLinearMultiClassifier
sparse/cross_assess_sparse
sparse/sweep_sparse
sparse/tune_sparse
sparse/SparseCrossReport
sparse/SparseTuneReport
```
<!-- apiref:toctree:end -->
