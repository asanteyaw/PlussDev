# `pluss::ai::StepOptions`

A [`Step`](Step.md)'s declared contract: its capability set, input/output
artifact names, required DataFrame columns, time-feature name, and the
`persistable`/`trained`/`can_change_row_count`/`requires_time`/
`learns_from_future` flags [`Procedure::check`](Procedure/check.md) and
[`Procedure::run`](Procedure/run.md) enforce.

Built, filled in, and attached to a [`Step`](Step.md) by every
[`procedure::step`](../build/step.md) overload -- a caller using a
built-in engine adapter rarely constructs one by hand; the generic
`step(name, options)`/`step(name, options, runner)` overloads are where a
caller fills every field directly.

## Fields

| Field | Meaning | Default | Enforcement point |
| --- | --- | --- | --- |
| `capabilities` | Declared roles supported by the step | `{StepCapability::Map}` | Graph validation and capability queries |
| `inputs` | Named artifacts consumed by the step | `{}` | Graph validation and run-time artifact resolution |
| `outputs` | Named artifacts the step may publish | `{}` | Link validation and default output naming |
| `require_columns` | Tabular columns that must be present | `{}` | Before the runner executes |
| `time_feature` | Column designated as the step's time axis | `""` | Before the runner when `requires_time=true` |
| `persistable` | Whether the graph may save this step | `false` | `Procedure::save()` |
| `trained` | Construction-time snapshot of component readiness | `false` | Metadata only |
| `can_change_row_count` | Permit output row count to differ from input | `false` | After the runner executes |
| `requires_time` | Require a valid `time_feature` in tabular input | `false` | Before the runner executes |
| `learns_from_future` | Declare intentional future-data access | `false` | Before non-inference execution |
| `component` | Stable adapter/component label for audit and persistence | `""` | Audit and saved manifest |

### `capabilities`

`std::vector<StepCapability>`, default `{StepCapability::Map}`. The
step's declared [`StepCapability`](Introduction.md) set.
[`Procedure::check`](Procedure/check.md) rejects a step with an empty
set; nothing else in `core` dispatches on these values -- they are a
declaration, read via [`Step::has`](Step/has.md)/[`capability_mask`](Step/capability_mask.md).

### `inputs`

`std::vector<std::string>`, default empty. The names
[`Step::run`](Step/run.md) expects to find already-produced artifacts
under, resolved from the run's artifact pool by
[`Link`](Link.md)/[`Procedure::run`](Procedure/run.md).
[`Procedure::check`](Procedure/check.md) rejects duplicate names within
one step's own list.

### `outputs`

`std::vector<std::string>`, default empty. The names a step's own result
may be published under. A step with no declared output falls back to
naming its result after itself (see [`Step::run`](Step/run.md)).

### `require_columns`

`std::vector<std::string>`, default empty. Column names a `DataFrame` or
`lymph::workflow::Payload` input must contain --
[`Procedure::run`](Procedure/run.md) throws before the step's own runner
even executes if any are missing. Leave empty for a step that does not
read tabular data, or whose shape is enforced some other way.

### `time_feature`

`std::string`, default empty. The column name
[`Procedure::run`](Procedure/run.md) treats as the time axis when
`requires_time` is set -- checked for presence the same way
`require_columns` entries are.

### `persistable`

`bool`, default `false`. Whether [`Procedure::save`](Procedure/save.md)
is allowed to include this step at all; every step in a graph must have
this `true` before the graph as a whole can be saved. Every built-in
engine adapter sets this automatically; a plain
[`callable`](../callable.md) step needs the
[`registry`](../registry.md)'s `CustomPersist` overload to become
persistable.

### `trained`

`bool`, default `false`. Whether the wrapped component already has
learned state, snapshotted at step-construction time from the
component's own `trained()` -- it does not update afterward even if
[`run`](Step/run.md) later calls the component's `learn`.

### `can_change_row_count`

`bool`, default `false`. Declares that this step is allowed to produce
an output with a different row count than its input.
[`Procedure::run`](Procedure/run.md) throws if a row-count change is
detected on a step that left this `false` -- a safety net against a
silent row-dropping bug.

### `requires_time`

`bool`, default `false`. Declares that `time_feature` must be present
(and, together with `learns_from_future`, factors into the
future-data-access guard) on every input [`Procedure::run`](Procedure/run.md)
resolves for this step.

### `learns_from_future`

`bool`, default `false`. Declares that this step's logic deliberately
looks ahead in time -- blocked by
[`Procedure::run`](Procedure/run.md) in any
[`ProcedureMode`](Introduction.md) but `Infer` unless the caller
explicitly sets [`RunOptions::allow_temporal_leakage`](RunOptions.md)
`true` for that call. Reserve this for a step whose future-data access is
genuinely intentional (an audited backtest feature), not a convenience
flag to silence the guard.

### `component`

`std::string`, default empty. A short tag identifying what kind of thing
this step wraps -- recorded in every
[`StepTrace`](StepTrace.md)/saved-bundle manifest. Every built-in adapter
sets a fixed value for its own kind; see [`Step::component`](Step/component.md).

## Guidance

Most callers never fill a `StepOptions` by hand for an engine-adapted
step -- [`procedure::step`](../build/step.md)'s
[`lymph`](../lymph.md)/[`ice`](../ice.md)/[`nlp`](../nlp.md)/[`module`](../module.md)
overloads build one correctly from the wrapped component. Hand-filling
one directly (via the generic `step(name, options)`/
`step(name, options, runner)` overloads) is normal for a
[`callable`](../callable.md) step or a bare graph-shaping placeholder.
`require_columns`/`time_feature`/`requires_time`/`learns_from_future` are
the production-safety fields worth setting deliberately on any step that
reads real tabular, time-ordered data, regardless of which adapter built
it.

## See Also

- [`Step`](Step.md) -- every accessor this struct backs.
- [`build`](../build.md) -- the generic `step` overloads a caller fills
  this with directly.
- [`RunOptions`](RunOptions.md) -- `allow_temporal_leakage`, the other
  half of the future-data-access guard.
