# `classic::io` Introduction

## What This Family Owns

`io` owns exactly one round trip: a [`Flow`](../compose/Flow.md) to a file,
and a file back to a `Flow`. `save` writes; `load` reads the file back as
plain data (a [`FlowBundle`](FlowBundle.md), no `Flow` built yet); `restore`
turns that data into a runnable `Flow`; `load_and_restore` does both in one
call, the common case.

## Two Ways A Bundle Restores A Fitted `Flow`

`save` tries to capture a trained `Flow`'s exact fitted parameters — every
stage's [`dump_transform_states`](../compose/Flow/dump_transform_states.md)
output and the model's own
[`dump_model_state`](../compose/Flow/dump_model_state.md) output, embedded
directly in the file. When that succeeds, `restore` calls the matching
`load_transform_states` / `load_model_state` and the restored `Flow` is
`trained()` immediately, with the identical parameters — no re-fitting.

A bundle can also carry the original training batch instead (or in
addition): `save`'s `train_batch` parameter. When there is no usable fitted
snapshot — the `Flow` was never trained, or its own state-dump path is
unavailable — `save` falls back to storing `train_batch` and
`train_options`, and `restore` re-runs `flow.learn(...)` on that same data
instead of loading parameters directly. Either way you get back a `Flow`
that behaves like the one you saved; only the mechanism differs. See
[`save`](save.md#parameters) for exactly which combination of "already
trained" and "batch supplied" produces which outcome, and
[`FlowBundle`](FlowBundle.md#state) for reading `has_snapshot` yourself to
tell which one a given file actually holds.

## A Bundle Is Self-Contained, Not Forward-Compatible

A saved file embeds everything `restore` needs — the
[`ModelSpec`](../families/ModelSpec.md), every
[`TransformSpec`](../compose/TransformSpec.md), and either fitted state or
training data — so restoring never touches the filesystem again beyond that
one path. What it does **not** do is tolerate a schema change: `load`
rejects any bundle whose stored schema version does not match the current
one exactly, with no partial or best-effort reading of an older format (see
[`load`'s Example 5](load.md#example-5-a-schema-version-mismatch)). Treat a
saved bundle as tied to the library version that wrote it, not as a
durable long-term interchange format.

## Reading Order

1. [`FlowBundle`](FlowBundle.md) — the data shape everything else produces
   or consumes.
2. [`save`](save.md) — writing one.
3. [`load`](load.md) — reading one back, as data (`load`) or as a runnable
   `Flow` (`restore` / `load_and_restore`).

## Related Pages

- [`compose`](../compose.md) — `Flow`, the type this family persists and
  rebuilds.
- [`estimators`](../estimators.md) — `EstimatorBase::dump_state` /
  `load_state`, the per-model half of a fitted-state snapshot.
- [`data`](../data.md) — `CategoryMaps`, carried in every bundle for a
  `Flow` fit from raw `Note` columns.
