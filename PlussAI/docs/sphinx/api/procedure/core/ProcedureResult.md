# `pluss::ai::ProcedureResult`

What [`Procedure::run`](Procedure/run.md) returns: every produced
[`Artifact`](Artifact.md), plus a human-readable trace, a structured
per-step audit, any non-fatal warnings, and which step failed, if any.
Constructed only by `run()` itself -- there is no public factory for
building one by hand.

## Signature

```cpp
pluss::ai::ProcedureResult::ProcedureResult();
```

## Construction

A `ProcedureResult` only ever comes from
[`Procedure::run`](Procedure/run.md); its own multi-argument constructor
is an implementation detail `run()` uses internally.

The public default constructor represents an empty successful result: there is
no failed step, so `ok()` is true, and every collection is empty. Normal user
code should consume the value returned by `run()` rather than manufacture
results as control-flow signals.

## State

[`ok()`](ProcedureResult/ok.md)/[`failed_step()`](ProcedureResult/failed_step.md)
-- whether the run completed and, if not, where;
[`artifacts()`](ProcedureResult/artifacts.md)/[`metrics()`](ProcedureResult/metrics.md)
-- the produced values;
[`trace()`](ProcedureResult/trace.md)/[`audit()`](ProcedureResult/audit.md)
-- what ran and how, at two levels of detail;
[`warnings()`](ProcedureResult/warnings.md) -- non-fatal messages
collected under `fail_fast = false`.

## Result Layers

The collections answer different questions:

| Layer | Purpose | Payload retained |
| --- | --- | --- |
| `artifacts()` | Values produced by successful steps before completion or failure | Full `Artifact` values |
| `metrics()` | Reserved metric artifacts separated from ordinary outputs | Full `Artifact` values |
| `trace()` | Compact execution order such as `"ran:score"` | Strings only |
| `audit()` | Component, input/output metadata, elapsed time, and row-count changes | Metadata snapshots only |
| `warnings()` | Diagnostics captured during non-fail-fast execution | Strings only |
| `failed_step()` | Name of the step that stopped execution | One string |

`ok()` means no step was recorded as failed. It does not mean that the result
contains a particular artifact or metric; use `get<T>(name)` for that contract.

## Partial Results

With `ProcedureOptions::fail_fast=false`, a failed run returns normally.
Artifacts from steps that completed before the failure remain available, while
the failing step and all remaining work produce nothing. This makes a
result useful for diagnostics and recovery without pretending the complete
graph succeeded. With `fail_fast=true`, no `ProcedureResult` is returned at all
because the exception propagates.

## Ownership And Access

The result owns its artifact and metadata vectors. `get<T>()` returns a const
reference into that owned storage, so the result must outlive the reference.
Looking up a missing name and requesting the wrong payload type are separate
errors with separate diagnostics. No implicit conversion is attempted.

## Methods

See the method pages below for `ok`/`failed_step`/`trace`/`audit`/
`warnings`/`artifacts`/`metrics`/`get`.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

ProcedureResult/Introduction
ProcedureResult/artifacts
ProcedureResult/audit
ProcedureResult/failed_step
ProcedureResult/get
ProcedureResult/metrics
ProcedureResult/ok
ProcedureResult/trace
ProcedureResult/warnings
```
<!-- apiref:toctree:end -->
