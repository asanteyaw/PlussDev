# `pluss::ai::ice::ExecutionPolicy`

Gradient clipping, epoch/accumulation/dev-batch limits, non-finite
failure, determinism, precision, and platform.

Nested in [`TrainOptions::execution`](TrainOptions.md) and every
`models_*` catalog entry's own `*TrainOptions` field of the same name.
Reuses `nn::optim`'s own `GradientClipOptions` directly rather than
re-declaring a parallel `ice`-native type -- same object model if a
caller drops to raw `nn` underneath. Entirely read by
[`Coach::learn`](../coach/Coach.md) and
[`make_optimizer`](../optim/make_optimizer.md).

## Fields

### `grad_clip`

[`GradientClipOptions`](../../nn/optim/GradientClipOptions.md) `grad_clip = {.mode = ByNorm, .threshold = 1.0}`
: Gradient clipping, passed straight to the constructed optimizer's own
  `Options::clip` field. On by default (clip-by-norm at threshold `1.0`)
  -- numerical safety is a default here, not an opt-in.

### `max_epochs`

`std::int64_t max_epochs = 60`
: The training loop's natural stopping point, reached regardless of
  early stopping. Combined with `fast_dev_batches` for a quick
  end-to-end smoke test: the loop still breaks after its first epoch
  when `fast_dev_batches > 0`.

### `accumulation_steps`

`std::int64_t accumulation_steps = 1`
: How many mini-batches' gradients are summed before an optimizer step.
  `1` (the default) steps after every batch. Raising it simulates a
  larger effective batch size without the memory cost of actually
  batching that many samples together. Must be positive.

### `fast_dev_batches`

`std::int64_t fast_dev_batches = 0`
: When `> 0`, overrides *both* `max_train_batches` and
  `max_valid_batches` to this same small count and stops the loop after
  one epoch -- a one-setting "does this pipeline actually run" smoke
  test, independent of `max_epochs`. `0` (the default) disables this
  override; `max_train_batches`/`max_valid_batches` (if set) apply as
  written instead. Must be non-negative.

### `max_train_batches`

`std::int64_t max_train_batches = 0`
: Caps how many training batches run per epoch. `0` (the default) runs
  every batch. Ignored when `fast_dev_batches > 0`. Must be
  non-negative.

### `max_valid_batches`

`std::int64_t max_valid_batches = 0`
: The same cap as `max_train_batches`, for the validation pass. Must be
  non-negative.

### `fail_on_non_finite`

`bool fail_on_non_finite = true`
: Maps to the constructed optimizer's own `StepDiagnosticsOptions`
  `fail_if_non_finite_sensitivity`/`fail_if_non_finite_parameter`/
  `fail_if_non_finite_loss`, all three at once. `true` (the default)
  fails loudly the moment a gradient, parameter, or loss value goes
  non-finite, instead of silently continuing to train on garbage.

### `deterministic`

`bool deterministic = false`
: When `true`, seeds the training loop's RNG deterministically from
  `TrainOptions::seed` (or the catalog entry's own `seed`), for
  reproducible shuffling/splitting given a fixed seed. `false` (the
  default) still accepts a `seed`, just without the same determinism
  guarantee across every source of randomness in the loop.

### `precision`

`ScalarType precision = ScalarType::Float32`
: The floating-point dtype training runs at. Only a floating dtype is
  honored -- an integer dtype silently resolves to `Float32` instead of
  throwing.

### `platform`

`TrainPlatform platform = TrainPlatform::CPU`
: Which device to train on -- see
  [`resolve_platform`](../platform/resolve_platform.md) for how `CPU`/
  `GPU`/`Auto` each resolve. `CPU` (the default) matches `ice`'s
  behavior before this option existed; opting into acceleration is
  always a deliberate choice.

## Guidance

Leave every field at its default for ordinary training -- clipping and
non-finite failure are already on, and `max_epochs = 60` is a reasonable
starting budget. Reach for `fast_dev_batches` while first wiring up a
new `Model`/dataset, to confirm the whole pipeline runs end to end
before committing to a full `max_epochs` run. Raise
`accumulation_steps` when a batch size large enough to train well does
not fit in memory as a single mini-batch. Set `platform =
TrainPlatform::GPU` (or `Auto`) to opt into acceleration; `infer`/
`assess` afterward automatically match wherever the trained model's
parameters ended up, with no need to pass a platform again.

## See Also

- [`TrainOptions`](TrainOptions.md) -- the field this nests under.
- [`GradientClipOptions`](../../nn/optim/GradientClipOptions.md) -- the
  shared clipping type `grad_clip` reuses.
- [`resolve_platform`](../platform/resolve_platform.md) -- resolves
  `platform` against real device availability.
- [`Coach::learn`](../coach/Coach.md) /
  [`make_optimizer`](../optim/make_optimizer.md) -- the two consumers.
