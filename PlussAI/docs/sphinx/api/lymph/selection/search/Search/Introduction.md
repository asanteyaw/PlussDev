# `lymph::selection::search::Search` Introduction

## What This Type Represents

`Search` is a **hyperparameter search held as a value**: a list of axes to sweep
and a split policy, configured once by a factory and run against a model-building
closure.

It is not a spec sweep. `run`'s first argument is a `make_model` closure that
returns a full `models::Model` — prep stages, branch structure, learner
hyperparameters, all of it — built from one `GridPoint`. Every trial therefore
cross-validates a real end-to-end pipeline. A trial can vary anything the
closure can express, not just numeric learner parameters.

## Why It Lives Under `search`

`search` owns hyperparameter search. `Search` is its whole surface — one driver
type plus its [`Result`](../Result.md). Sibling `selection` concerns:

- `selection::compare` ranks a fixed list of already-built
  candidates.
- [`selection::spotcheck`](../../spotcheck.md) sweeps a training-strategy ×
  model grid from a `Builder`.
- `selection::tune_specs` is the `ModelSpec`-grid analogue, for when a trial
  never needs prep.

## Reading Order

1. Construction — the [`grid` / `random` / `build`](../grid.md) factories
2. [`run`](run.md) — the sweep itself
3. [`Result`](../Result.md) — `board()`, `best()`, `top(k)`

## Notes

### The `make_model` Closure

Read axis values with `point.at("axis_name")`. Attach prep inside the closure
(`m.standard_scaler()`, `m.prepare()...`) — it is refit per fold, so there is no
train/test leakage.

### Cost

Every trial rebuilds and refits its whole model, prep included, once per fold —
the same per-candidate cost `selection::compare` pays.
`random` trades coverage for a fixed trial budget when the grid is large.

### Split Policy

The policy lives on the `Search`. A single `run` call can still pass an explicit
`SplitOptions` to override it — useful for a quick holdout pass before the full
CV sweep.

## Related Pages

- [`selection::search`](../../search.md) — the subfamily
- [`GridAxis` / `GridPoint`](../grid.md) — the grid coordinate types
- [`CompareReport`](../../CompareReport.md) — the ranked-trial report inside
  `Result`

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="run.html">
    <span class="label">Next</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
