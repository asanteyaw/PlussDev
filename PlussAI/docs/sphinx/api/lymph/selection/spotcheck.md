# Lymph `selection::spotcheck`

`spotcheck` is the `selection` subfamily for a **quick strategy × model grid** —
take the models already registered in a [`models::Builder`](../models/builder/Builder.md),
cross them with a few training strategies, and rank every combination by
cross-validation.

| Entity | Role |
| --- | --- |
| [`Suite`](spotcheck/Suite.md) | the driver — [`strategy(name, …)`](spotcheck/Suite/strategy.md) adds a training strategy; [`run(...)`](spotcheck/Suite/run.md) cross-validates every strategy × model pair |
| `suite(builder)` | builds a `Suite` from a `models::Builder` (documented on the [`Suite`](spotcheck/Suite.md) page) |

`run` returns a `spotcheck::Result` (an alias of
[`selection::CompareResult`](CompareReport.md)) — the ranked board, the best and
worst combinations, and the fitted models.

This is the fastest way to answer "of the modelling directions I've registered,
which one — under which training strategy — should I take further?" without
writing a hyperparameter search.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

spotcheck/Introduction
spotcheck/Suite
```
<!-- apiref:toctree:end -->
