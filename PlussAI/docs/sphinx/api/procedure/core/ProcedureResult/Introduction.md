# `pluss::ai::ProcedureResult` Introduction

## What `ProcedureResult` Is

`ProcedureResult` is [`Procedure::run`](../Procedure/run.md)'s return
value: every [`Artifact`](../Artifact.md) produced during the run (the
seeded inputs plus every step's own output, in the order each was first
added to the pool), a lightweight `"ran:<name>"` [`trace`](trace.md), a
structured per-step [`audit`](audit.md) (when
[`RunOptions::audit`](../RunOptions.md) was left at its default `true`),
any [`warnings`](warnings.md) collected under
[`ProcedureOptions::fail_fast`](../ProcedureOptions.md) `= false`, and
[`failed_step`](failed_step.md) naming which step stopped the run, if
any.

## Two Levels Of Run History

`trace()` and `audit()` answer the same underlying question --
"what happened?" -- at different levels of detail and cost.
[`trace`](trace.md) is always collected and is just a list of
`"ran:<name>"` strings: cheap, always present, good for a quick log line.
[`audit`](audit.md) is a full [`StepTrace`](../StepTrace.md) per step
(every input's and the output's [`ArtifactTrace`](../ArtifactTrace.md),
elapsed microseconds, and whether row count changed) and can be switched
off per call via `RunOptions::audit = false` when a caller wants the
speed without the bookkeeping.

## `metrics()` Is Reserved

`metrics()` returns the same shape as [`artifacts()`](artifacts.md) --
a separate list for steps that report a metric distinct from their main
output -- but [`Procedure::run`](../Procedure/run.md) currently never
populates it; every call returns an empty list. Documented here as a
reserved, forward-looking accessor rather than silently omitted, since it
is a real, callable part of the public surface today.

## Related Pages

- [`Procedure::run`](../Procedure/run.md) -- the only way to obtain a
  `ProcedureResult`.
- [`Artifact`](../Artifact.md) -- what `artifacts()`/`get()` actually
  hold.
- [`StepTrace`](../StepTrace.md) -- the full record type behind
  `audit()`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="artifacts.html">
    <span class="label">Next</span>
    <span class="title">artifacts</span>
  </a>
</div>
<!-- apiref:nav:end -->
