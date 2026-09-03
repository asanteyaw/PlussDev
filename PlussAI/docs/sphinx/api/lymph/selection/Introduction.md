# `selection` Introduction

## What This Family Owns

`selection` owns the public surfaces that turn "which model / which settings?"
into a reproducible experiment:

- **[`validate`](validate.md)** — `Plan` and `HoldoutPlan`, built by
  [`cross`](validate/cross.md), `stratified`, `grouped`,
  [`time`](validate/time.md), and [`holdout`](validate/holdout.md). A plan is a
  reusable contract: fold count, shuffling, seeding, worker count, and (for
  chronological plans) gap / horizon / lookback. Running it against a model and
  a frame yields a [`reports::CrossVal`](../metrics/CrossValidationReport.md) (aka `metrics::CrossValidationReport`).
- **[`search`](search.md)** — `Search`, built by `grid` / `random` / `build`
  over a set of `GridAxis`. It calls a `make_model(GridPoint)` factory per grid
  point, cross-validates each, and returns a `search::Result` with the best
  model and `top(k)`.
- **[`spotcheck`](spotcheck.md)** — `Suite`, from `suite(builder)`. Each
  `strategy(name, TrainOptions)` crossed with each model in the builder becomes
  one candidate; `run` ranks them all.

Umbrella-level, [`compare`](compare.md) ranks a fixed list of `Candidate`s
(name + `models::Model` + optional `TrainOptions`) — model *selection* without a
parameter search — and returns a [`CompareReport`](CompareReport.md).
[`GridAxis` / `GridPoint`](GridAxis.md) are the search-space coordinate types
`search` and spec-grid tuning share. [`tune_specs`](tune_specs.md) (and its
custom-sampler variant `probe_specs`) tune a `ModelSpec` grid instead of
pre-built models.

## The Three Jobs, Kept Distinct

| You have… | and you want to… | reach for |
| --- | --- | --- |
| one model + a frame | an honest score under a validation policy | [`validate`](validate.md) |
| a `make_model` factory + a parameter grid | the best point in that grid | [`search`](search.md) |
| a `models::Builder` + a few training strategies | the best strategy × model pair | [`spotcheck`](spotcheck.md) |
| a fixed list of already-built models | them ranked against each other | [`compare`](compare.md) |

## Namespace

Everything canonical lives under `lymph::selection`. The
`selection::validate` / `selection::search` / `selection::spotcheck`
sub-namespaces hold the definitions and are re-exported to `selection::`, so
both `lymph::selection::validate::cross(...)` and the shorter
`lymph::selection::cross(...)` name the same function. Option structs also carry
the umbrella spelling `lymph::options::CrossVal`.

## Related Pages

- [`models`](../models.md) — the artifacts `selection` compares.
- [`models::Builder`](../models/builder/Builder.md) — the candidate workspace
  `spotcheck::Suite` and `Builder::benchmark` draw from.
- [`reports::CrossVal`](../metrics/CrossValidationReport.md) — what
  a validation `Plan::run` produces.
