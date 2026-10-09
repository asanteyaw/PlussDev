# `pluss::ai::ArtifactTrace`

A lightweight, type-aware snapshot of one [`Artifact`](Artifact.md)
captured during a run: its name, resolved type name, and (where known)
row/column/element counts and definedness.

Built internally by [`Procedure::run`](Procedure/run.md) for every input
and output of every step, and exposed read-only as part of each
[`StepTrace`](StepTrace.md) in
[`ProcedureResult::audit`](ProcedureResult/audit.md) -- there is no
public factory; a caller only ever reads one that `run()` already built.

## Fields

### `name`

`std::string`, default empty. The artifact's own
[`name()`](Artifact/name.md) at the moment it was traced.

### `type`

`std::string`, default empty. A human-readable type name --
`"Series"`, `"DataFrame"`, `"lymph::FeatureBatch"`, and so on, one per
alternative [`Artifact::Value`](Artifact/Introduction.md) can hold, or
`"empty"` for an artifact with no value.

### `rows`

`std::int64_t`, default `-1`. The artifact's leading-dimension count,
where the type has one (a `Series`'s `sizes().front()`, a `DataFrame`'s
`ncases()`, and similar) -- `-1` for a type this concept does not apply
to.

### `columns`

`std::int64_t`, default `-1`. The artifact's trailing-dimension or
feature count, where the type has one -- `-1` otherwise.

### `elements`

`std::int64_t`, default `-1`. A total element count, where computable
(a `Series`'s `numel()`, a `DataFrame`'s `rows * columns`, and similar)
-- `-1` otherwise.

### `defined`

`bool`, default `true`. Whether the underlying value itself is in a
defined state (an allocated `Series`, for instance) -- distinct from
[`Artifact::empty`](Artifact/empty.md), which asks whether the variant
holds any alternative at all.

## Guidance

`ArtifactTrace` exists so [`StepTrace`](StepTrace.md) can record what an
artifact *looked like* without keeping the artifact itself alive in the
audit trail -- a run's
[`ProcedureResult::audit`](ProcedureResult/audit.md) stays cheap to hold
onto even for large `Series`/`DataFrame` traffic. Any field left at its
sentinel default (`-1` for the three counts) simply means that concept
did not apply to the artifact's type, not that it failed to compute.

## See Also

- [`StepTrace`](StepTrace.md) -- where every `ArtifactTrace` is found,
  one per input plus one for the output.
- [`Artifact`](Artifact.md) -- the full value this is a snapshot of.
