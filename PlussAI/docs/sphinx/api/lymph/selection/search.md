# Lymph `selection::search`

`search` is the `selection` subfamily for **hyperparameter search** — finding
the best point in a parameter grid by cross-validating a model built at each
point.

| Entity | Role |
| --- | --- |
| [`Search`](search/Search.md) | the driver — [`run`](search/Search/run.md) calls a `make_model` closure at each grid point, cross-validates that full model, and ranks the trials |
| [`Result`](search/Result.md) | what `run` returns — `.report` (a [`CompareReport`](CompareReport.md)), `.best()`, `.top(k)` |
| [`grid` / `random` / `build`](search/grid.md) | the `Search` factories — exhaustive, sampled, and `TuneOptions`-driven |

An axis is a `GridAxis` (`{name, values}`); one combination is a `GridPoint`,
which the `make_model` closure turns into a `models::Model`. Because the closure
builds a *full* model — prep stages included — every trial cross-validates a
genuine end-to-end pipeline.

For sweeping a `ModelSpec` grid that never needs prep, see
`selection::tune_specs`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

search/Introduction
search/Search
search/Result
search/grid
```
<!-- apiref:toctree:end -->
