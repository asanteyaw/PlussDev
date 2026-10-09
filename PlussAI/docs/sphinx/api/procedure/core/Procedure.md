# `pluss::ai::Procedure`

A validated, immutable DAG of [`Step`](Step.md)s and [`Link`](Link.md)s.
Build one with [`procedure::make`](../build/make.md), then call
[`run`](Procedure/run.md) once per call to execute it and get back a
[`ProcedureResult`](ProcedureResult.md).

## Signature

```cpp
pluss::ai::Procedure();
pluss::ai::Procedure(std::vector<Step> steps, std::vector<Link> links, ProcedureOptions options = {});

pluss::ai::Procedure pluss::ai::procedure::make(std::vector<Step> steps, std::vector<Link> links = {}, ProcedureOptions options = {});
pluss::ai::Procedure pluss::ai::procedure::make(std::initializer_list<Step> steps, std::initializer_list<Link> links = {}, ProcedureOptions options = {});
pluss::ai::Procedure pluss::ai::procedure::load(const std::string& path);
```

## Construction

A default-constructed `Procedure` is empty and never checked. Every other
`Procedure` comes from one of three places: the constructor directly, the
[`procedure::make`](../build/make.md) free functions (the normal path --
they just forward to the constructor), or [`procedure::load`](../persist/load.md)
(reconstructs one from a bundle a prior `save()` wrote). Unless
[`ProcedureOptions::validate_on_make`](ProcedureOptions.md) is set `false`,
construction calls [`check`](Procedure/check.md) immediately, so a `Procedure`
value is either valid or never came into existence.

Prefer `procedure::make` at call sites. It keeps graph construction visually
separate from the `Procedure` value type and accepts both vectors and
initializer lists. Use the direct constructor when a builder already owns
`std::vector<Step>` and `std::vector<Link>` and moving them directly is clearer.
Use `procedure::load` only for a bundle previously written by `save`; it
reconstructs runners and component state before applying the same graph
validation as an in-memory build.

The default constructor creates an empty inspection value. It has no steps,
links, artifacts, or hidden executor. `empty()` is true, but `check()`,
`run()`, and `save()` reject it because a valid procedure must contain at least
one step. This distinguishes a representable default C++ value from an
executable deployment graph.

## State

Two containers (`steps()`, `links()`), the options it was built with
(`options()`), and three cheap summaries (`step_count()`, `link_count()`,
`empty()`) derived from them. Nothing else is stored -- `Procedure` holds
no artifacts and no run history of its own; those live only in the
[`ProcedureResult`](ProcedureResult.md) each `run()` call returns.

## Immutability And Reuse

The graph topology and `ProcedureOptions` are immutable after construction.
This prevents a link or step from changing between validation and execution.
The components captured by steps may still hold intentional mutable state:
learning adapters update their owned model when run in `Learn` mode. Thus,
"immutable Procedure" means immutable graph structure, not that every wrapped
model is stateless.

Each `run()` creates fresh artifacts, trace entries, audit records, warnings,
and failure state. Reusing a procedure does not append to an earlier result.
Independent calls should not be assumed safe against the same stateful model
unless that model's own contract permits concurrent access.

## Topology Versus Artifact Routing

`Link` establishes dependency order between step names. `StepOptions::inputs`
and `outputs` establish artifact names. A simple one-input/one-output chain can
omit explicit port names on its links. A join or multi-port step should name
both ends so validation can prove that the upstream output satisfies the
intended downstream input. External artifacts supplied to `run()` seed the
same name-based pool and can satisfy an input without an incoming link.

Graph validation is structural. It catches duplicate step names, duplicate
ports, unknown endpoints, invalid named ports, empty capability sets, cycles,
and non-positive worker counts. Artifact type compatibility remains a run-time
check because one procedure may legitimately accept different artifact types
or run modes on different calls.

## Cost Model

Construction is linear in the number of steps and links plus cycle detection.
`run()` performs another structural check, builds dependency bookkeeping, and
then executes each step once. Artifact payloads use value semantics defined by
their own types; inexpensive handle types such as `Series` do not imply a deep
element copy merely because they enter an `Artifact`. Structured audit adds
metadata snapshots, not copies of full payloads.

## Methods

See the method pages below for `steps`/`links`/`options`/`step_count`/
`link_count`/`empty`/`names`/`has`/`get`/`check_message`/`check`/`run`/`save`.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Procedure/Introduction
Procedure/check
Procedure/check_message
Procedure/empty
Procedure/get
Procedure/has
Procedure/link_count
Procedure/links
Procedure/names
Procedure/options
Procedure/run
Procedure/save
Procedure/step_count
Procedure/steps
```
<!-- apiref:toctree:end -->
