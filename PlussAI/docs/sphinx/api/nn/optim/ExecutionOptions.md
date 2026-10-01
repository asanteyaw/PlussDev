# `pluss::ai::nn::optim::ExecutionOptions`

Controls how a parameter group's update is actually computed -- as a plain
per-parameter scalar loop, or as one vectorized op over the whole group
packed along a new batch axis -- plus strictness and CUDA-graph-capture
safety. Every concrete optimizer's own `Options` struct ends with an
`execution` field of this type, shared across all seven exactly like
[`GradientClipOptions`](GradientClipOptions.md).

Passed as part of an optimizer's own `Options` struct; the default,
`ExecutionOptions{}` (`lane = Auto`), lets the optimizer pick the fastest
lane its parameters actually support.

## Fields

### `lane`

`UpdateLane lane = UpdateLane::Auto`
: Which update lane to run:
  - `Auto` (the default) -- resolves to `Batched` if the group's
    parameters support it, else `Fused` if that is supported, else falls
    back to `Loop`. This is almost always the right choice; the other
    three values exist mainly for testing, benchmarking, or working
    around a platform-specific limitation.
  - `Loop` -- always the plain per-parameter scalar loop, regardless of
    what a batched or fused path could do. Always supported, on every
    platform and dtype this reference's optimizers accept.
  - `Foreach` -- routed through the same vectorized path as `Batched` at
    dispatch time; kept as a separate named value mainly for parity with
    other frameworks' terminology.
  - `Batched` -- packs every active parameter (and its gradient, and its
    per-parameter state) in the group into one `Series` with a new
    leading batch axis, and runs the whole group's update as a single op.
    Requires every active parameter in the group to share platform,
    dtype, and shape.
  - `Fused` -- a single fused elementwise graph over the packed group,
    where the platform supports it.

### `strict`

`bool strict = false`
: What happens when a requested lane (`Foreach`/`Batched`/`Fused`, or
  `capturable` when unsupported) cannot actually run for the current
  group. `false` (the default) silently falls back to `Loop`. `true`
  throws instead -- reach for this while developing or benchmarking, to
  be told immediately when an assumption about what a group supports
  turns out to be wrong, rather than quietly getting a slower lane than
  expected.

### `capturable`

`bool capturable = false`
: Enables CUDA-graph-capture-safe execution: hyperparameters that would
  otherwise be read once on the host (learning rate, step-dependent bias
  corrections, ...) are instead advanced as on-device scalar `Series`
  values every step, so the whole update sequence can be captured into a
  CUDA graph and replayed without re-reading host memory. Rejected for
  complex parameters (every optimizer's own group-options validation
  throws if `capturable` is set alongside a complex parameter). Only
  relevant when actually using CUDA graph capture; leave at the default
  `false` otherwise.

## Guidance

Leave `lane` at `Auto` for ordinary training -- it already picks the
fastest lane every parameter in a group can actually support, and falls
back safely when they cannot. Set `strict = true` while iterating on a
model's construction (mixed platforms or dtypes within one parameter
group are the most common reason a requested lane silently falls back) so
a mismatch surfaces immediately instead of only showing up as an
unexplained slowdown. Reach for `capturable` only alongside actual CUDA
graph capture; it has no effect and no cost otherwise beyond the
capturable-state bookkeeping it enables.

## See Also

- [`GradientClipOptions`](GradientClipOptions.md) -- the other field every
  concrete optimizer's `Options` ends with.
- [`StepLaneTelemetry`](StepLaneTelemetry.md) -- reports which lane
  `Auto` (or any other request) actually resolved to on the most recent
  step.
- [`Optimizer::last_step_lane_telemetry`](Optimizer/last_step_lane_telemetry.md)
  -- how to read that report back.
