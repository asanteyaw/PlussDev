# `pluss::ai::ice::CheckpointPolicy`

Whether/when to write checkpoints during training, where, and how to
resume.

Nested in [`TrainOptions::checkpoint`](TrainOptions.md) and `models_ts`'s
two `*TrainOptions` (`models_nlp`/`models_rl` do not use it). Entirely
read by [`Coach::learn`](../coach/Coach.md) -- every `Model::learn`
overload delegates to `Coach` internally, so this field takes effect
there too.

## Fields

### `enabled`

`bool enabled = false`
: The master switch. `false` (the default) means none of the other
  fields below have any effect -- no checkpoint is ever written, and
  `resume_from` is required to be empty (set alongside `enabled =
  false` throws).

### `save_last`

`bool save_last = false`
: When `true`, writes a `<stem>-last.ckpt` checkpoint every
  `every_n_epochs` epochs, overwriting the previous one. Requires
  `enabled = true`.

### `save_best`

`bool save_best = false`
: When `true`, writes a `<stem>-best.ckpt` checkpoint every time
  `ValidationPolicy::monitor` improves (by
  [`LoopRegularization::min_delta`](LoopRegularization.md), in the
  direction [`ValidationPolicy::direction`](ValidationPolicy.md) says),
  overwriting the previous best. Requires `enabled = true` and that
  `monitor` is actually produced on every checkpointed epoch (throws
  otherwise).

### `save_weights_only`

`bool save_weights_only = false`
: When `true`, each checkpoint write calls `Model::save` (parameters
  only, via [`ice::io::save_parameters`](../io/save_parameters.md))
  instead of [`ice::io::save_checkpoint`](../io/save_checkpoint.md)
  (the full resumable artifact: parameters, optimizer state, recipe,
  epoch/step counters, callback state). A weights-only checkpoint can be
  loaded into a fresh model for inference, but cannot be resumed from --
  `resume_from` pointing at one throws.

### `save_on_exception`

`bool save_on_exception = false`
: When `true`, a training-loop exception triggers one additional
  `<stem>-exception.ckpt` write (a full resumable checkpoint,
  independent of `save_weights_only`) before the exception propagates,
  so a crashed run can still be resumed from its last completed epoch.
  Requires `enabled = true`.

### `every_n_epochs`

`std::int64_t every_n_epochs = 1`
: How often (in epochs) `save_last` writes -- independent of
  [`ValidationPolicy::every_n_epochs`](ValidationPolicy.md), which
  governs the *validation pass* cadence instead. `save_best` is not
  gated by this field; it writes on every improving validation epoch,
  whatever that epoch's number. Must be positive.

### `dirpath`

`std::string dirpath = ""`
: The directory checkpoints are written under. Empty (the default) uses
  the current working directory. Created automatically if it does not
  exist.

### `filename`

`std::string filename = ""`
: The checkpoint filename stem, before the `-best.ckpt`/`-last.ckpt`/
  `-exception.ckpt` suffix. Empty (the default) uses `"ice-checkpoint"`.

### `resume_from`

`std::string resume_from = ""`
: A checkpoint path to resume training from. Empty (the default) starts
  fresh. Requires `enabled = true` and `save_weights_only = false`, and
  either `save_best = false` or `save_weights_only = false` together
  with it (resuming while also configured to overwrite a best-metric
  weights-only checkpoint is rejected as an unsafe combination).

## Guidance

Leave `enabled = false` (the default) for a quick run with nothing
written to disk. Set `enabled = true` with `save_best = true` for the
common "keep the best model seen" case; add `save_last = true` alongside
it when being able to resume an interrupted run matters more than disk
space. Reach for `save_weights_only = true` only when checkpoints are
purely for later inference, never for resuming this exact training run --
`resume_from` cannot point at one. `save_on_exception = true` is cheap
insurance for long runs where a crash partway through would otherwise
lose all progress.

## See Also

- [`TrainOptions`](TrainOptions.md) -- the field this nests under.
- [`Coach::learn`](../coach/Coach.md) -- the consumer.
- [`save_checkpoint`](../io/save_checkpoint.md) /
  [`load_checkpoint`](../io/load_checkpoint.md) -- the full artifact
  format written when `save_weights_only = false`.
- [`ValidationPolicy`](ValidationPolicy.md) -- `monitor`/`direction`,
  read by `save_best`.
