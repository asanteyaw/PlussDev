# `pluss::ai::ice::LoopRegularization`

Early stopping, patience, minimum delta, and best-weight restoration.

Nested in [`TrainOptions::loop_regularization`](TrainOptions.md) and
every `models_*` catalog entry's own `*TrainOptions` field of the same
name; read entirely by
[`EarlyStopCallback`](../callback/EarlyStopCallback.md), alongside
[`ValidationPolicy`](ValidationPolicy.md)'s `monitor`/`direction`, on
every validation epoch.

## Fields

### `early_stopping`

`bool early_stopping = true`
: When `true` (the default), training stops once `patience` validation
  epochs in a row fail to improve `monitor` by at least `min_delta`.
  `false` disables stopping entirely -- `wait_count`/`best_value`/
  `best_epoch` are still tracked and readable off
  [`EarlyStopCallback`](../callback/EarlyStopCallback.md), training just
  never ends early because of them.

### `patience`

`std::int64_t patience = 10`
: How many consecutive non-improving validation epochs are tolerated
  before stopping, when `early_stopping` is `true`. Counted in
  validation epochs (governed by
  [`ValidationPolicy::every_n_epochs`](ValidationPolicy.md)), not raw
  epochs.

### `min_delta`

`double min_delta = 1e-6`
: The minimum change in `monitor` that counts as an improvement --
  `current < best - min_delta` when minimizing, `current > best +
  min_delta` when maximizing (see
  [`ValidationPolicy::direction`](ValidationPolicy.md)). Raise this to
  ignore noise-level fluctuations that would otherwise reset the
  patience counter.

### `restore_best_weights`

`bool restore_best_weights = false`
: When `true`, a snapshot of every registered parameter is taken on
  every improving validation epoch; at the end of training, the model's
  parameters are restored to that best snapshot (overwriting whatever
  the final epoch left them at). `false` (the default) leaves the
  model's parameters at their final-epoch values, whether or not that
  was the best epoch. When resuming from a checkpoint with no in-memory
  snapshot carried over, restoration instead reloads from
  `RunState::best_checkpoint_path`.

## Guidance

Leave `early_stopping = true` with the default `patience`/`min_delta`
for a reasonable first run; raise `patience` for a noisy validation
metric that improves in bursts rather than monotonically. Set
`restore_best_weights = true` whenever overfitting past the best epoch
is a real risk -- without it, a model trained past its best epoch keeps
the worse, final-epoch weights even though the better ones were seen
and could have been kept. This field has no effect when `early_stopping
= false` on its own: the restoration still happens at the natural end
of training (`max_epochs` reached), it just isn't the *reason* training
ended.

## See Also

- [`TrainOptions`](TrainOptions.md) -- the field this nests under.
- [`ValidationPolicy`](ValidationPolicy.md) -- `monitor`/`direction`,
  read together with this struct.
- [`EarlyStopCallback`](../callback/EarlyStopCallback.md) -- the
  consumer; its own accessors (`best_value`, `best_epoch`,
  `stopped_epoch`, `wait_count`) expose what this struct's fields
  produced during the most recent run.
