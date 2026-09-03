# Lymph `selection`

`selection` is the lymph family for **model selection** — deciding, with
evidence, which model or which settings should survive. The `models` family owns
the trainable artifacts; `selection` owns the rules under which they are
compared and judged.

It has three subfamilies plus a small set of umbrella entry points:

| Subfamily | Concern |
| --- | --- |
| [`selection::validate`](selection/validate.md) | how evidence is earned — cross-validation plans, holdout, grouped and chronological validation |
| [`selection::search`](selection/search.md) | hyperparameter search over a grid — `grid` / `random` / `build` feeding a `Search` |
| [`selection::spotcheck`](selection/spotcheck.md) | a quick strategy × model grid from a `models::Builder` |

Umbrella level (directly under `selection::`):

| Entity | Role |
| --- | --- |
| [`compare`](selection/compare.md) | rank a set of already-built `Candidate`s by cross-validation |
| [`CompareReport`](selection/CompareReport.md) | the ranked-trial report `compare` / `Search` / `Suite` all return, plus the `Trial` / `CompareResult` value types |
| [`GridAxis` / `GridPoint`](selection/GridAxis.md) | grid coordinates shared by `search` and spec-grid tuning; `grid_size` / `grid_point_at` / `grid_points` |
| [`tune_specs`](selection/tune_specs.md) | tune a `ModelSpec` grid via a `make_spec` closure; `probe_specs` is the custom-sampler variant |

The three jobs are deliberately kept distinct: `validate` produces a *plan* and
runs it; `search` explores a *parameter space*; `spotcheck` and `compare` rank a
*fixed set of candidates*. The `selection::validate::` / `selection::search::` /
`selection::spotcheck::` sub-namespaces are re-exported to `selection::` directly,
so `selection::cross(...)`, `selection::grid(...)`, and `selection::suite(...)`
all work unqualified within `selection`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

selection/Introduction
selection/compare
selection/CompareReport
selection/GridAxis
selection/tune_specs
selection/validate
selection/search
selection/spotcheck
```
<!-- apiref:toctree:end -->
