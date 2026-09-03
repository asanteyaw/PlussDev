# `selection::search` Introduction

## What This Subfamily Owns

`search` owns hyperparameter search over a grid:

- **[`Search`](Search.md)** — the driver. [`grid(axes, split|cv)`](grid.md)
  builds an exhaustive `Search`; [`random(axes, trials, …)`](grid.md) a sampled
  one; [`build(axes, TuneOptions)`](grid.md) a fully-configured one.
  [`run(make_model, data, …)`](Search/run.md) calls `make_model(GridPoint)` at
  each point, cross-validates the resulting model, and ranks the trials by an
  objective metric.
- **[`Result`](Result.md)** — `run`'s return. `.report` is a
  [`CompareReport`](../CompareReport.md); `.best()` is the winning
  `models::Model`, re-fitted on the whole dataset; `.top(k)` hands back the k
  best as `(name, Model)` pairs (cap `k` at `FitOptions.fit_top`).

## Search vs. Compare vs. Spotcheck

`search` explores a *parameter space* you describe with axes. It does not need
you to build the models — the `make_model` closure does that per point. Use
`selection::compare` when you already hold a fixed list of
built models, and [`selection::spotcheck`](../spotcheck.md) when the grid is
"every training strategy × every model in a `Builder`".

## Related Pages

- [`grid` / `random` / `build`](grid.md) — the factories, plus the `GridAxis` /
  `GridPoint` coordinate types.
- [`CompareReport`](../CompareReport.md) — the ranked-trial report inside
  `Result`.
- `selection::tune_specs` — the `ModelSpec`-grid analogue.
