# `lymph::selection::search::Search`

`Search` is the **hyperparameter-search driver** of the
[`selection::search`](../search.md) subfamily. It holds a search space (a list of
[`GridAxis`](grid.md)) and a split policy as a value; its one method,
[`run`](Search/run.md), visits the space, cross-validates a model built at each
point, and ranks the trials.

The improvement over a bare spec sweep: `run` takes a `make_model` closure that
builds a **full `models::Model`** — prep stages included — from each grid point,
so every trial cross-validates a genuine end-to-end pipeline, not an unscaled
estimator core.

## Scope And Alias

The type is `lymph::selection::search::Search`, re-exported one scope up as
`lymph::selection::Search`. Both names are the same type.

```cpp
namespace lymph = pluss::ai::lymph;

lymph::selection::Search a = lymph::selection::grid(axes, cv);
lymph::selection::search::Search b = lymph::selection::search::random(axes, 20, cv);
```

## Construction

Built by a factory, not by hand — its constructor is private. All three live in
`lymph::selection::search` and re-export to `lymph::selection`:

`grid(std::vector<GridAxis> axes, SplitOptions | CrossValOptions)`
: Exhaustive — one trial per point in the cross product of the axes.

`random(std::vector<GridAxis> axes, std::int64_t trials, …)`
: Samples `trials` points from the space. Overloads take a `RandomState` +
  `SplitOptions`, or a `CrossValOptions` (which also seeds the sampler), or
  both.

`build(std::vector<GridAxis> axes, TuneOptions options)`
: From an assembled `TuneOptions` — search kind, trial count, seed, split,
  worker count in one struct.

[`grid` / `random` / `build`](grid.md) has the full signatures and worked
examples.

## Methods

| Method | Summary |
|--------|---------|
| [`run`](Search/run.md) | Call `make_model` at each grid point, cross-validate that full model, rank the trials → [`search::Result`](Result.md). Many data-shape overloads, each with an optional `SplitOptions` override. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Search/Introduction
Search/run
```
<!-- apiref:toctree:end -->

## Changelog

- **0.1** — `Search` with `grid` / `random` / `build` factories and `run`
  (model-factory × [`LabeledBatch` / `(x, y)` / `(frame, data_options)` /
  `(workflow, payload)`] × [stored split | explicit `SplitOptions`]).
