# `selection::spotcheck` Introduction

## What This Subfamily Owns

`spotcheck` owns the strategy × model comparison:

- **[`Suite`](Suite.md)** — built by `suite(builder)`. It reads every model the
  [`models::Builder`](../../models/builder/Builder.md) holds. Each
  [`strategy(name, …)`](Suite/strategy.md) call adds one training strategy (a
  balance target, calibration, class weights, an iteration budget, …).
  [`run`](Suite/run.md) forms one candidate per strategy × model, cross-validates
  them all, and returns a `CompareResult` — the ranked board plus the best/worst
  fitted models.

## Spotcheck vs. Builder::benchmark

[`Builder::benchmark`](../../models/builder/Builder/benchmark.md) already ranks a
builder's models under a single training configuration. `spotcheck::Suite` adds
the *strategy* axis: the same models compared under several training strategies
at once, so you see not just "which model" but "which model under which
settings". When one strategy is enough, `Builder::benchmark` is the shorter
path.

## Related Pages

- [`models::Builder`](../../models/builder/Builder.md) — the model source.
- [`CompareReport`](../CompareReport.md) — the report type inside the result.
- `selection::compare` — rank a hand-built candidate list instead.
