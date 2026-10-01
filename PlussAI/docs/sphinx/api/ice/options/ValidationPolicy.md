# `pluss::ai::ice::ValidationPolicy`

Validation split ratio, evaluation cadence, and the monitored metric
name/direction for early stopping and best-checkpoint tracking.

Nested in [`TrainOptions::validation`](TrainOptions.md) and every
`models_*` catalog entry's own `*TrainOptions` field of the same name;
read by the training loop (to decide whether this epoch runs a
validation pass at all) and by
[`EarlyStopCallback`](../callback/EarlyStopCallback.md) (to decide
whether that pass's result is an improvement).

## Fields

### `split_ratio`

`double split_ratio = 0.1`
: The fraction of training data carved out as a validation set, for the
  two `Series`-pair overloads of `Model::learn` (and the equivalent
  overloads across the `models_*` catalog). `0.0` disables validation
  entirely -- no split, no validation-loss metric produced, and
  `EarlyStopCallback`/checkpoint-by-best-metric have nothing to act on.
  Ignored by the already-split `(x_train, y_train, x_val, y_val)`
  overload and by the `Source`-taking overload, both of which supply
  their own validation data directly.

### `every_n_epochs`

`std::int64_t every_n_epochs = 1`
: How often (in epochs) the validation pass actually runs:
  `(epoch - start_from_epoch) % every_n_epochs == 0`. `1` (the default)
  validates every epoch. Raising it skips the validation forward pass on
  the epochs in between, trading finer-grained monitoring for less
  compute spent on evaluation rather than training. Must be positive.

### `start_from_epoch`

`std::int64_t start_from_epoch = 0`
: The first epoch (0-indexed) eligible for a validation pass --
  epochs before this are skipped entirely, regardless of
  `every_n_epochs`. Useful for skipping validation (and therefore early
  stopping) during an initial warm-up stretch where the monitored metric
  is expected to be noisy or uninformative.

### `monitor`

`std::string monitor = "val_loss"`
: The metric name `EarlyStopCallback` reads via `RunState::metric(monitor)`
  every validation epoch. `"val_loss"` (the default) is always produced
  when validation runs; a model or callback that records additional
  named metrics onto `RunState` can be monitored by name instead (e.g.
  `"val_accuracy"`).

### `direction`

`MonitorDirection direction = MonitorDirection::Auto`
: Whether an improvement means `monitor` going down (`Min`), up (`Max`),
  or should be inferred from the name (`Auto`, the default).
  `Auto` treats any name containing `acc`, `auc`, `f1`, `precision`,
  `recall`, `map`, `ndcg`, or `score` (case-insensitive) as `Max`, and
  everything else -- including the default `"val_loss"` -- as `Min`.
  Set this explicitly for a custom metric name `Auto`'s heuristic would
  guess wrong for.

## Guidance

Leave `split_ratio` at its default `0.1` for a first run; raise it for a
noisier validation signal with a smaller training set, or set it to
`0.0` when validation isn't wanted at all (no split-ratio value disables
just early stopping while keeping validation loss around -- the two are
the same mechanism here). `every_n_epochs`/`start_from_epoch` matter
mainly when validation itself is expensive relative to a training step;
otherwise leave both at their defaults (validate every epoch, from
epoch 0). Set `direction` explicitly whenever `monitor` is a custom
metric name `Auto`'s substring heuristic might not classify correctly --
getting this wrong silently inverts what "improvement" means for both
early stopping and best-checkpoint selection.

## See Also

- [`TrainOptions`](TrainOptions.md) -- the field this nests under.
- [`LoopRegularization`](LoopRegularization.md) -- `early_stopping`/
  `patience`/`min_delta`, read together with this struct by
  `EarlyStopCallback`.
- [`EarlyStopCallback`](../callback/EarlyStopCallback.md) -- the
  consumer of `monitor`/`direction`.
- [`CheckpointPolicy`](CheckpointPolicy.md) -- its own, separate
  `every_n_epochs` cadence for writing checkpoints.
