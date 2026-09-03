# Lymph `reports`

`reports` is the lymph family for **model-backed evaluation** and the **index of
every report type** in the framework.

Two jobs:

1. **Facade** — take a trained [`models::Model`](models/Model.md) (or a
   [`workflow::Workflow`](workflow.md)) plus a dataset, and get the same
   evidence [`metrics`](metrics.md) computes from raw `Series` — with the
   inference done for you. `model.reports().classification_summary(frame)`
   instead of `metrics::classification_summary(y_true, model.infer(x))`.
2. **Umbrella aliases** — one canonical short name for each report type,
   regardless of which family defines it.

## The Report Type Index

Every `reports::` alias, its canonical short spelling, and where the type is
actually defined:

| `reports::` alias | Underlying type | Home family | Produced by |
| --- | --- | --- | --- |
| `reports::Metric` | `metrics::MetricReport` | [`metrics`](metrics.md) | every `.report()` accessor |
| `reports::Class` | `metrics::ClassReport` | [`metrics`](metrics.md) | `classification_summary` |
| `reports::CrossVal` | `metrics::CrossValidationReport` | [`metrics`](metrics.md) | [`Plan::run`](selection/validate/Plan/run.md) |
| `reports::SeriesCompare` | `metrics::SeriesCompareReport` | [`metrics`](metrics.md) | `compare_series` |
| `reports::Compare` | `selection::CompareReport` | [`selection`](selection.md) | [`compare`](selection/compare.md), `Search`, `Suite` |
| `reports::Tune` | `TuneReport` | [`selection`](selection.md) | [`tune_specs`](selection/tune_specs.md) |
| `reports::Model` | `reports::ModelReports` | `reports` | [`model.reports()`](reports/Model.md) |

Use the `reports::` spelling in cross-family code; use the home-family spelling
on that family's own pages.

## The Facade Surface

| Entity | What it does |
| --- | --- |
| [`reports::Model`](reports/Model.md) | `model.reports()` — one accessor bundling the helpers below for a given model |
| [`classification_summary` / `confusion_table`](reports/classification_summary.md) | `(model | workflow, data)` → the classification tables, inference included |
| [`threshold_curve` / `best_threshold`](reports/threshold_curve.md) | `(model | workflow, data)` → the operating-point sweep without materializing scores yourself |

## Relationship To `metrics`

`reports` is a thin layer over [`metrics`](metrics.md). Each facade call runs
the model to get predictions (or scores), then hands off to the matching
`metrics` compute helper. If you already have the prediction `Series`, skip
`reports` and call `metrics` directly.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

reports/Introduction
reports/Model
reports/classification_summary
reports/threshold_curve
```
<!-- apiref:toctree:end -->
