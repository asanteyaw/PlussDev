# `pluss::ai::StepTrace`

One step's audit record from a run: its name/component, every input's
[`ArtifactTrace`](ArtifactTrace.md), the output's `ArtifactTrace`,
elapsed microseconds, and whether it changed row count.

Built internally by [`Procedure::run`](Procedure/run.md), one per step
that ran, and exposed read-only via
[`ProcedureResult::audit`](ProcedureResult/audit.md) when
[`RunOptions::audit`](RunOptions.md) is `true` (the default) -- there is
no public factory; a caller only ever reads one that `run()` already
built.

## Fields

### `step`

`std::string`, default empty. The step's own [`name()`](Step/name.md).

### `component`

`std::string`, default empty. The step's own
[`component()`](Step/component.md) tag.

### `inputs`

`std::vector<ArtifactTrace>`, default empty. One
[`ArtifactTrace`](ArtifactTrace.md) per artifact the step was run
against, in the same order [`Step::run`](Step/run.md) received them.

### `output`

`ArtifactTrace`, default-constructed. A snapshot of the step's own
result artifact.

### `elapsed_us`

`std::int64_t`, default `0`. Wall-clock microseconds the step's own
[`run`](Step/run.md) call took. Under
[`ProcedureOptions::workers`](ProcedureOptions.md) `> 1`, this is the
individual step's own duration, not the wave's -- several steps running
concurrently each report their own elapsed time independently.

### `row_count_changed`

`bool`, default `false`. `true` when any input's
[`ArtifactTrace::rows`](ArtifactTrace.md) differs from the output's --
the condition [`Procedure::run`](Procedure/run.md) checks against
[`Step::can_change_row_count`](Step/can_change_row_count.md) before
allowing the run to continue.

## Guidance

`StepTrace` is the structured form of run history
[`ProcedureResult::audit`](ProcedureResult/audit.md) exposes --
`row_count_changed` in particular is worth inspecting on any step that
legitimately reshapes data, since it is the same signal
[`Procedure::run`](Procedure/run.md) itself enforces against
`can_change_row_count`.

## See Also

- [`ArtifactTrace`](ArtifactTrace.md) -- the per-artifact snapshot type
  `inputs`/`output` hold.
- [`ProcedureResult::audit`](ProcedureResult/audit.md) -- where a list
  of these is found.
- [`Step::can_change_row_count`](Step/can_change_row_count.md) -- the
  declared contract `row_count_changed` is checked against.
