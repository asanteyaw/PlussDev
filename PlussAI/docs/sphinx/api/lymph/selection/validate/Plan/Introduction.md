# `lymph::selection::validate::Plan` Introduction

## What This Type Represents

`Plan` is a **validation policy held as a value**: fold count, shuffling,
seeding, worker count, and — for chronological plans — the time-window shape. You
configure it once and run it against as many models and frames as you like.

It is not "options plus a function". It is the object that guarantees every
candidate in a comparison is judged under *identical* folds, seeds, and worker
counts — which is the whole point of a fair comparison. `selection::search` and
`selection::spotcheck` both take a `Plan` (or its options) for exactly this
reason.

## Why It Lives Under `validate`

`validate` owns the reusable validation contracts. `Plan` is the k-fold one;
`HoldoutPlan` is the single-split one. Both are built by factories, not by hand:

- [`cross`](../cross.md) — plain k-fold
- `stratified` — class-balanced folds (imbalanced labels)
- `grouped` — folds respect a group key (leak-prone repeated identities)
- [`time`](../time.md) / `time_ordered` — chronological, expanding window

## Reading Order

1. Construction and configuration — on the [`Plan`](../Plan.md) landing page
2. [`run`](run.md) — fit and score across the folds
3. [`scores`](scores.md) — one metric per fold, or its mean
4. [`splits`](splits.md) — the fold boundaries of a `time` plan, without fitting

## Notes

### It Holds Only Options

Copying a `Plan` is cheap. The same configured plan judges every candidate in a
comparison under the same folds and seeds — pass it around freely.

### `run` Does Not Train Your Model

`run` is `const`. It clones the model per fold; the instance you hand it stays
untrained.

### Time-Only Surface

`gap` / `horizon` / `lookback`, and `splits` / `last`, raise on a non-`time`
plan rather than silently doing nothing.

## Related Pages

- [`selection::validate`](../../validate.md) — the subfamily
- [`options::CrossVal`](../CrossValOptions.md) — the option fields
- [`reports::CrossVal`](../../../metrics/CrossValidationReport.md) — the report
  `run` returns

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="run.html">
    <span class="label">Next</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
