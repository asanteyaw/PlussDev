# `selection::validate` Introduction

## What This Subfamily Owns

`validate` owns the reusable validation contracts:

- **[`Plan`](Plan.md)** — a cross-validation contract. Built by
  [`cross`](cross.md) (plain k-fold), `stratified` (class-balanced folds),
  `grouped` (folds respect a group key), or [`time`](time.md) (chronological,
  expanding window). Chained setters — `folds`, `repeat`, `shuffle`, `seed`,
  `workers`, and `gap` / `horizon` / `lookback` for time plans — refine it.
  `run(model, frame, data_options)` returns a
  [`reports::CrossVal`](../../metrics/CrossValidationReport.md);
  `scores(...)` returns just the per-fold metric `Series`.
- **`HoldoutPlan`** — a single train/test split contract. Built by
  [`holdout`](holdout.md); `test_size`, `stratify`, `shuffle`, `seed` refine it;
  `split(frame)` produces a `HoldoutSplit` with `train_set` / `test_set`.

## Why A Plan, Not A Function Call

A validation policy is a property of the *experiment*, not of one model. Holding
it as a value means every candidate in a comparison is judged under identical
folds, seeds, and worker counts — which is the whole point of a fair
comparison. The `selection::search` and `selection::spotcheck` drivers take a
`CrossValOptions` (or a `Plan`) for exactly this reason.

## Related Pages

- [`reports::CrossVal`](../../metrics/CrossValidationReport.md) (`metrics::CrossValidationReport`) —
  the report `Plan::run` returns.
- [`options::CrossVal`](CrossValOptions.md) — the option fields.
- [`selection::search`](../search.md) / [`selection::spotcheck`](../spotcheck.md)
  — drivers that consume a validation policy.
