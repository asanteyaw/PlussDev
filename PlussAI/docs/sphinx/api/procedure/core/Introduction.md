# `procedure::core` Introduction

## What This Category Owns

`core` owns the five types every `procedure` program is built from --
[`Procedure`](Procedure.md) (the validated DAG), [`Step`](Step.md) (one
node in it), [`Link`](Link.md) (one edge), [`Artifact`](Artifact.md) (the
typed value flowing along an edge), and [`ProcedureResult`](ProcedureResult.md)
(what a run produces) -- plus the plain option/record structs that
configure and describe them
([`ProcedureOptions`](ProcedureOptions.md), [`RunOptions`](RunOptions.md),
[`StepOptions`](StepOptions.md), [`StepPersistSpec`](StepPersistSpec.md),
[`SinkSpec`](SinkSpec.md), [`ArtifactTrace`](ArtifactTrace.md),
[`StepTrace`](StepTrace.md)). No other category introduces a new core type;
`build`, `callable`, `lymph`, `ice`, `nlp`, `module`, and `registry` all
produce and consume exactly these five.

## The Four Small Enums

Four scoped enums appear across `core`'s structs. None gets its own page --
each is small enough, and tied closely enough to one field, to read in
place:

- `ProcedureSchedule` (`ProcedureOptions::schedule`) -- `Deterministic`
  (the default; steps run in one fixed topological order every call) or
  `Adaptive` (reserved for a future scheduler that may reorder independent
  steps; currently behaves the same as `Deterministic`).
- `ProcedureCache` (`ProcedureOptions::cache`) -- `Off` (the default) or
  `ReadyArtifacts` (reserved for a future artifact cache keyed on upstream
  inputs; currently has no effect on `run()`).
- `ProcedureMode` (`RunOptions::mode`) -- `Infer` (the default),
  `Learn`, or `Assess`. Every adapter `Step`'s runner switches on this to
  decide whether to call the wrapped component's `learn`/`infer`/`assess`
  method; a plain [`callable`](../callable.md)/[`module`](../module.md)
  step ignores it entirely, since those have only one behavior.
- `StepCapability` (`StepOptions::capabilities`, a `std::vector` acting as
  a set) -- `Admit`/`Map`/`Learn`/`Infer`/`Assess`/`Gate`/`Join`/`Sink`/
  `Watch`. A capability is a declaration, not a dispatch switch:
  [`Procedure::check`](Procedure/check.md) requires every step to declare
  at least one, and [`Step::has`](Step/has.md) lets a caller or a future
  scheduler ask what a step is for, but nothing in `core` itself restricts
  *which* capability a step may combine with which input/output shape --
  see [`StepOptions`](StepOptions.md)'s own Guidance for the conventional
  meaning of each value.

## Reading Order

1. [`Procedure`](Procedure.md) -- the whole picture: a step list, a link
   list, and `run()`.
2. [`Step`](Step.md) -- what one node actually is underneath `procedure::step`'s
   many convenience overloads (see [`build`](../build.md)).
3. [`Link`](Link.md) -- the plain edge struct `procedure::link` builds.
4. [`Artifact`](Artifact.md) -- the type-erased value every step
   consumes and produces.
5. [`ProcedureResult`](ProcedureResult.md) -- `run()`'s return value: every
   artifact plus the trace/audit/warnings describing how it was produced.
6. [`ProcedureOptions`](ProcedureOptions.md) / [`RunOptions`](RunOptions.md) /
   [`StepOptions`](StepOptions.md) -- the three option structs a caller
   fills in directly.
7. [`StepPersistSpec`](StepPersistSpec.md) / [`SinkSpec`](SinkSpec.md) /
   [`ArtifactTrace`](ArtifactTrace.md) / [`StepTrace`](StepTrace.md) --
   the smaller records `Step`/`ProcedureResult` build internally and expose
   read-only.

## Related Pages

- [`build`](../build.md) -- the free functions (`step`/`sink`/`link`/
  `input`/`make`) that construct these types; `core` only documents the
  types themselves.
- [`persist`](../persist.md) -- `Procedure::save`/`procedure::load`'s
  bundle format, which `StepPersistSpec` and `StepTrace` both feed.
