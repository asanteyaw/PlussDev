# `pluss::ai::classic::ConfusionOptions`

The normalization mode, label order, and sample weights consumed by
[`confusion_table`](confusion_table.md).

## Fields

### `normalize`

`ConfusionNormalize normalize = ConfusionNormalize::None`

How to scale the raw counts:

- `None` (the default) — raw counts, as tallied.
- `ByTruth` — each row divided by its own sum, so every row (one true
  class) sums to `1` — the fraction of that class's rows predicted into
  each column. A row with zero true occurrences stays all-zero rather than
  dividing by zero.
- `ByPrediction` — each column divided by its own sum, so every column (one
  predicted class) sums to `1`. A column with zero predictions stays
  all-zero.
- `Global` — every count divided by the grand total, so the whole matrix
  sums to `1`.

### `label_order`

`pluss::Series label_order = Series::undefined()`

The class values, and the row/column order they appear in. Undefined (the
default) uses the sorted union of every value seen in `truth_labels` and
`predicted_labels` — the class set is discovered, not declared. Supply this
explicitly to fix the class set and its order (needed when a class might be
absent from one particular call's data but should still get a row/column of
zeros, or when you want a specific display order). Must contain no
duplicates when supplied.

### `sample_weights`

`pluss::Series sample_weights = Series::undefined()`

Per-row weights contributing to the counts — a weighted confusion matrix
instead of a plain tally. Undefined (the default) weights every row `1`.
Must be finite, non-negative, and match `truth_labels`'s length when
supplied.

## Guidance

Leave `normalize = None` to inspect raw counts; switch to `ByTruth` when you
want each class's own recall-style breakdown at a glance (the diagonal
entry of a `ByTruth`-normalized row is that class's recall). Supply
`label_order` explicitly whenever you are comparing matrices across
multiple calls (different batches, different folds) and need the row/column
meaning to line up identically every time — the default sorted-union
behavior can silently produce a different-shaped matrix if one call's data
happens to be missing a class another call's data has.

## See Also

- [`confusion_table`](confusion_table.md) — the consuming function.
- [`ConfusionTable`](ConfusionTable.md) — the returned type, which carries
  the resolved `label_order` and `normalize` back.
- [`ClassReportOptions`](ClassReportOptions.md) — the equivalent options for
  the fuller classification report.
