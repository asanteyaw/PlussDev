# `pluss::ai::ice::ObjectiveRegularization`

Weight decay (coupled or decoupled), L1/L2 penalties, and label smoothing
applied to the training objective.

Nested in [`TrainOptions::objective_regularization`](TrainOptions.md)
and every `models_*` catalog entry's own `*TrainOptions` field of the
same name. Two different mechanisms read it: `weight_decay`/
`decoupled_weight_decay` are consumed by
[`make_optimizer`](../optim/make_optimizer.md) (folded into the
optimizer's own construction), while `l1_penalty`/`l2_penalty`/
`label_smoothing` are applied by `Model::compute_loss`'s context-aware
overload (added directly to the computed loss, or threaded into the
loss function itself).

## Fields

### `weight_decay`

`double weight_decay = 0.0`
: The optimizer's own weight decay strength, passed straight to
  whichever algorithm `TrainOptions::optimizer` selects. Must be
  non-negative.

### `decoupled_weight_decay`

`bool decoupled_weight_decay = false`
: When `true`, decay is applied directly to the parameter (decoupled,
  AdamW-style) rather than folded into the gradient. Only `AdamW` and
  `NAdam` support this; `make_optimizer` throws if it is set `true`
  alongside any other `optimizer` value.

### `l1_penalty`

`double l1_penalty = 0.0`
: Adds `l1_penalty * sum(abs(parameter))` (summed over every registered
  parameter) to the loss. `0.0` (the default) adds nothing. Must be
  non-negative.

### `l2_penalty`

`double l2_penalty = 0.0`
: Adds `l2_penalty * sum(parameter^2)` (summed over every registered
  parameter) to the loss, independent of `weight_decay` -- the two
  mechanisms can be used together (one adjusts the optimizer's update
  rule, the other adjusts the loss value itself) or separately. Must be
  non-negative.

### `label_smoothing`

`double label_smoothing = 0.0`
: Only takes effect when `TrainOptions::loss`/the catalog entry's own
  loss is `LossKind::CrossEntropy`: softens the one-hot target
  distribution before computing cross-entropy, the same smoothing
  [`nn::losses::CrossEntropyOptions::label_smoothing`](../../nn/losses/CrossEntropyOptions.md)
  applies. `0.0` (the default) is plain cross-entropy. Must be in
  `[0, 1]`.

## Guidance

`weight_decay`/`decoupled_weight_decay` and `l1_penalty`/`l2_penalty`
solve the same general problem (discourage large parameter values)
through two different mechanisms -- prefer `weight_decay` for the common
case (it is what every optimizer's own decay term is designed around),
and reach for `l1_penalty`/`l2_penalty` when a mechanism applied to the
loss value itself, rather than the update rule, is specifically wanted
(for example, to make the penalty visible in a logged loss curve).
`label_smoothing` only matters for classification trained with
`CrossEntropy`; it is silently inert for `MSE`/`BCEWithLogits` and for
any model that overrides `compute_loss` without reading it.

## See Also

- [`TrainOptions`](TrainOptions.md) -- the field this nests under.
- [`make_optimizer`](../optim/make_optimizer.md) -- applies
  `weight_decay`/`decoupled_weight_decay`.
- [`Model::compute_loss`](../model/Model/compute_loss.md) -- applies
  `l1_penalty`/`l2_penalty`/`label_smoothing`.
