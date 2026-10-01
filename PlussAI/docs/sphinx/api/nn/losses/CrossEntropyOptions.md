# `pluss::ai::nn::losses::CrossEntropyOptions`

Reduction mode, class dimension, label smoothing, per-class weight, and
an ignorable target index.

Passed to [`CrossEntropy`](CrossEntropy.md)'s third argument, defaulted
-- `CrossEntropy(logits, target)` alone is equivalent to
`CrossEntropy(logits, target, {})`.

## Fields

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element losses are combined. See
  [`Introduction`](Introduction.md#the-shared-reduction-mode).

### `class_dim`

`std::int64_t class_dim = -1`
: Which axis of `logits` holds the per-class values -- identical in
  meaning to [`NLLOptions::class_dim`](NLLOptions.md#class_dim). `-1`
  (the default) means the last axis.

### `label_smoothing`

`double label_smoothing = 0.0`
: Blends the class-index target's implicit one-hot distribution with a
  uniform distribution over classes before computing the loss: the
  effective per-element loss becomes `(1 - label_smoothing) *
  nll_loss + label_smoothing * mean(-log_probs, class_dim)`. Must be in
  `[0, 1]`. `0.0` (the default) is ordinary cross-entropy with no
  smoothing; values closer to `1.0` push the target distribution closer
  to uniform, which tends to make a model less overconfident (a smaller
  gap between the predicted probability of the correct class and every
  other class) at some cost to raw accuracy. Only meaningful for
  class-index targets -- see [`What It Does`](CrossEntropy.md#what-it-does)
  for what happens with a soft-probability target instead.

### `class_weight`

`Series class_weight = pluss::series::undefined()`
: An optional `[num_classes]` per-class weight. For class-index targets,
  behaves identically to
  [`NLLOptions::class_weight`](NLLOptions.md#class_weight) (including
  the weighted-mean denominator). For soft-probability targets, each
  class's contribution to the per-element loss is scaled by its own
  weight before summing across classes, rather than gathered by a single
  target index.

### `use_ignore_index`

`bool use_ignore_index = false`
: When `true`, elements whose `target` equals `ignore_index` are
  excluded, identically to
  [`NLLOptions::use_ignore_index`](NLLOptions.md#use_ignore_index). Only
  valid for class-index targets -- setting this `true` together with a
  soft-probability `target` throws, since a soft target has no single
  index value to compare against `ignore_index`. See
  [`CrossEntropy`](CrossEntropy.md#example-4).

### `ignore_index`

`std::int64_t ignore_index = -100`
: The target value treated as "ignore this element" when
  `use_ignore_index == true`. Identical in meaning to
  [`NLLOptions::ignore_index`](NLLOptions.md#ignore_index).

## Guidance

Everything [`NLLOptions`](NLLOptions.md#guidance)'s guidance says about
`class_dim`/`class_weight`/`use_ignore_index`/`ignore_index` applies
identically here for class-index targets. `label_smoothing` is the field
unique to this struct -- leave it at `0.0` unless overconfidence is a
known problem for the task at hand; a small value (`0.1` is a common
starting point) is the usual choice when it helps at all. Remember that
`use_ignore_index`/`ignore_index` require a class-index target --
reaching for both label smoothing and padding-position exclusion
together on soft-probability targets is not supported; mask or drop
those positions before calling instead.

## See Also

- [`CrossEntropy`](CrossEntropy.md) -- the only consumer of this options
  struct.
- [`NLLOptions`](NLLOptions.md) -- the four fields shared with this
  struct, without `label_smoothing` or soft-target support.
- [`BCEWithLogitsOptions`](BCEWithLogitsOptions.md) -- the binary
  (two-class) analogue.
