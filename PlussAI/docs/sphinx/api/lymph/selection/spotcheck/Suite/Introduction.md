# `lymph::selection::spotcheck::Suite` Introduction

## What This Type Represents

`Suite` is a **strategy × model spot-check held as a value**: a snapshot of a
`models::Builder`'s models plus a list of named training strategies. `run`
expands that into one candidate per pairing, cross-validates them all under one
policy, and ranks them.

It is not a hyperparameter search — there is no grid of learner parameters. The
axis it adds over [`Builder::benchmark`](../../../models/builder/Builder/benchmark.md)
is the *training strategy*: the same models judged under several `TrainOptions`
(balance targets, calibration, class weights, iteration budgets) at once.

## Why It Lives Under `spotcheck`

`spotcheck` owns exactly this: a quick, wide look before committing to a
direction. `Suite` is its whole surface — one driver type, its
[`strategy`](strategy.md) accumulator, and its [`run`](run.md) method.
`run` returns a `selection::CompareResult`, the same report type
[`selection::compare`](../../CompareReport.md) and
[`Plan::run(builder, …)`](../../validate/Plan/run.md) produce.

## Reading Order

1. Construction — `suite(builder)`, on the [`Suite`](../Suite.md) landing
2. [`strategy`](strategy.md) — register the strategies to cross with the models
3. [`run`](run.md) — the spot-check, and reading its `CompareResult`

## Notes

### Candidate Names

Each pairing is named `"<strategy>__<model>"` — that string is what appears in
the ranked board and in `top(k)` / `bottom(k)`. Choose short, comparable
strategy names.

### Nothing Runs Until `run`

The suite only holds model entries and the strategy list. All fitting happens
inside `run`, per fold, per candidate.

### Errors

No models on the builder, or no strategy registered, raises at `run` time.

## Related Pages

- [`selection::spotcheck`](../../spotcheck.md) — the subfamily
- [`models::Builder`](../../../models/builder/Builder.md) — the model source
- [`CompareReport`](../../CompareReport.md) — the report inside the result

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="run.html">
    <span class="label">Next</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
