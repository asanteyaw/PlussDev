# `classic::tune` Introduction

## What This Family Owns

`tune` turns a set of candidate [`ModelSpec`](../families/ModelSpec.md)s into
a ranked [`TuneReport`](TuneReport.md). Every function here evaluates each
candidate the same way — one [`metrics::cross_assess`](../metrics.md) call —
and differs only in *which* candidates get evaluated and in what order:

- [`sweep`](sweep.md) — an explicit list, every candidate, once each.
- [`probe`](probe.md) — a sampler callback, called once per trial to
  generate a candidate on demand, for a search space too large (or
  continuous) to enumerate by hand.
- [`tune`](tune.md) — either shape (list or sampler), routed through a
  [`TuneOptions`](TuneOptions.md) `search` setting to `sweep`, `probe`, or
  one of two strategies neither of those two functions implements on their
  own: successive halving (eliminate the worse half each round) and a
  Gaussian-process-guided Bayesian search.

Nothing here builds a long-lived estimator you keep — a trial's fitted
models exist only for the duration of its `cross_assess` call. What comes
back is data: a [`TuneReport`](TuneReport.md) recording every trial's
`ModelSpec`, its `Status`, its full `CrossAssessReport`, and the one
`objective` value it was ranked by. Reach for [`estimators`](../estimators.md)
once you already know which `ModelSpec` won and want to build and keep it.

## One Trial, One `cross_assess` Call

A "trial" always means the same thing: build a fresh estimator from a
`ModelSpec`, run [`cross_assess`](../metrics.md) against the shared batch and
[`SplitOptions`](../split/SplitOptions.md), and read one metric back out of
the resulting `CrossAssessReport.mean_metrics`. Every `ModelSpec` in a
`sweep`/`tune` candidate list is scored against **the same split plan** —
the comparison is fair because every candidate sees the same folds, not
because the candidates are similar to each other. A trial's own failure
(an invalid spec, a fold that could not fit, an objective metric absent
from the resolved metric list) never aborts the surrounding
`sweep`/`probe`/`tune` call — it is recorded on that one
[`TuneTrialResult`](TuneReport.md#state) and ranked last.

## Picking `objective_metric`

Every function in this family defaults `objective_metric` to
`MetricKind::Accuracy` — a classification metric. Calling any of them on a
**regression** task without explicitly passing a regression `MetricKind`
(`R2`, `Mse`, `Rmse`, `Mae`, or `QuantileLoss`) makes every single trial
fail, since `Accuracy` never appears among a regression task's resolved
metrics — see [`sweep`](sweep.md#example-3-the-default-objective_metric-pitfall)
for exactly what that failure looks like. There is no dedicated regression
default; always pass `objective_metric` explicitly for a regression sweep.

## Reading Order

1. [`sweep`](sweep.md) — the simplest shape: a fixed candidate list, no
   search strategy involved.
2. [`probe`](probe.md) — the sampler-driven shape, once `sweep`'s
   trial/ranking mechanics are familiar.
3. [`tune`](tune.md) — both shapes together, routed through
   [`TuneOptions`](TuneOptions.md) to `sweep`, `probe`, successive halving,
   or Bayesian search.
4. [`TuneReport`](TuneReport.md) / [`TuneLedger`](TuneLedger.md) — what
   comes back from any of the three.

## Related Pages

- [`metrics`](../metrics.md) — `cross_assess` / `CrossAssessReport`, what
  one trial runs and returns.
- [`split`](../split.md) — `SplitOptions`, the split plan every trial in one
  call shares.
- [`families`](../families.md) — `ModelSpec`, the candidate type this whole
  family ranks.
