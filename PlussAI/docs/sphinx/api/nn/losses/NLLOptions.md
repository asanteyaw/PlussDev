# `pluss::ai::nn::losses::NLLOptions`

Reduction mode, class dimension, per-class weight, and an ignorable
target index.

Passed to [`NLL`](NLL.md)'s third argument, defaulted -- `NLL(log_probs,
target)` alone is equivalent to `NLL(log_probs, target, {})`.

## Fields

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element losses are combined. Unlike the regression losses
  in this family, `Mean` here does **not** simply divide by the element
  count whenever `class_weight` is defined -- see that field below.
  Otherwise identical in meaning to
  [`Introduction`](Introduction.md#the-shared-reduction-mode).

### `class_dim`

`std::int64_t class_dim = -1`
: Which axis of `log_probs` holds the per-class log-probabilities.
  `-1` (the default) means the last axis. Negative values count from the
  end, the same convention every other axis-taking call in this
  reference uses. `target` must have exactly one fewer rank than
  `log_probs`, with every other axis matching -- see
  [`NLL`](NLL.md#example-4) for a `class_dim` other than the last axis.

### `class_weight`

`Series class_weight = pluss::series::undefined()`
: An optional `[num_classes]` per-class weight, gathered per element by
  that element's own target class. Undefined by default (every class
  weighted equally, `1.0`). When defined, `reduction=Mean` divides by
  the **sum of the gathered weights** for the active elements, not by
  the raw active-element count -- the same weighted-mean convention a
  class-imbalance-aware loss needs, so a heavily up-weighted rare class
  does not get diluted by simple averaging. See
  [`NLL`](NLL.md#example-2) for the exact arithmetic.

### `use_ignore_index`

`bool use_ignore_index = false`
: When `true`, every element whose `target` equals `ignore_index` is
  excluded entirely: its per-element loss is `0` (not merely excluded
  from the reduction -- the `None`-reduction output is literally `0`
  there), and it contributes to neither the `Sum` nor the (weighted or
  unweighted) `Mean`'s denominator. When `false` (the default),
  `ignore_index` is not consulted at all -- every element counts. See
  [`NLL`](NLL.md#example-3).

### `ignore_index`

`std::int64_t ignore_index = -100`
: The target value treated as "ignore this element" when
  `use_ignore_index == true`. Has no effect at all when
  `use_ignore_index == false` -- the default `-100` is simply a
  conventional sentinel value, not itself special until opted into.

## Guidance

Leave `class_dim` at its default `-1` for an ordinary `[batch, classes]`
or `[..., classes]` layout; set it explicitly only when the class axis is
not last (a `[batch, classes, seq_len]` sequence-tagging layout, for
instance). Reach for `class_weight` under class imbalance; reach for
`use_ignore_index`/`ignore_index` together (both are required --
`ignore_index` alone does nothing) when some target positions are padding
or otherwise meaningless and must not contribute to the loss at all,
such as padded positions in a batch of variable-length sequences.

## See Also

- [`NLL`](NLL.md) -- the only consumer of this options struct.
- [`CrossEntropyOptions`](CrossEntropyOptions.md) -- the same four
  fields, plus `label_smoothing` and support for soft-probability
  targets.
