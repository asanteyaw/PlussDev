# `pluss::ai::nn::losses::BCEWithLogitsOptions`

Reduction mode, per-element weight, and the positive-class weight.

Passed to [`BCEWithLogits`](BCEWithLogits.md)'s third argument, defaulted
-- `BCEWithLogits(logits, target)` alone is equivalent to
`BCEWithLogits(logits, target, {})`.

## Fields

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element losses are combined into the returned value. See
  [`Introduction`](Introduction.md#the-shared-reduction-mode) for the
  full three-way explanation shared by every loss in this family.

### `weight`

`Series weight = pluss::series::undefined()`
: An optional per-element multiplier applied to every element's loss
  after everything else (including `positive_class_weight`) has been
  applied -- `per = per.mul(weight)`. Undefined by default (no scaling
  at all). Must be a training-float dtype and share `logits`'s platform.
  Broadcastable against `logits`'s shape -- a single scalar weight
  scales every element identically (see
  [`BCEWithLogits`](BCEWithLogits.md#example-2)); a full per-element
  tensor lets each element carry its own independent weight (a common
  use: down-weighting examples known to be less reliable).

### `positive_class_weight`

`Series positive_class_weight = pluss::series::undefined()`
: An optional multiplier applied **only** to the positive-class (`target
  == 1`) term of the loss, leaving the negative-class (`target == 0`)
  term completely untouched. Undefined by default (no scaling). Used to
  counteract class imbalance: with far fewer positive examples than
  negative ones, a `positive_class_weight` greater than `1` makes each
  positive example's loss contribute proportionally more, without
  changing how negative examples are treated at all -- see
  [`BCEWithLogits`](BCEWithLogits.md#example-3) for exactly which
  elements this does and does not affect.

## Guidance

Leave `weight`/`positive_class_weight` undefined (the default) for a
balanced binary classification problem. Reach for `positive_class_weight`
specifically when positive examples are rare relative to negative ones,
scaled roughly to the negative/positive ratio; reach for `weight` when
individual examples (regardless of their class) carry different
importance or reliability. The two are independent and can be combined --
`positive_class_weight` shapes the positive-vs-negative balance,
`weight` then applies on top of that, per element.

## See Also

- [`BCEWithLogits`](BCEWithLogits.md) -- the only consumer of this
  options struct.
- [`CrossEntropyOptions`](CrossEntropyOptions.md) /
  [`NLLOptions`](NLLOptions.md) -- the multi-class analogues, which use
  a `class_weight` vector instead of a single scalar-or-broadcastable
  positive-class weight, since there is more than one class to balance.
