# `nn::losses` Introduction

## What This Category Owns

`losses` owns the stateless loss functions -- plain functions, not
`Module`s, each taking a prediction/logits/log-probabilities Series and a
target Series and returning a loss Series. Two groups:

- **Regression** (a continuous target): [`MSE`](MSE.md),
  [`RelativeMSE`](RelativeMSE.md), [`Huber`](Huber.md), [`L1`](L1.md),
  [`SmoothL1`](SmoothL1.md).
- **Classification** (a discrete or probability-distribution target):
  [`BCEWithLogits`](BCEWithLogits.md) (two classes),
  [`NLL`](NLL.md) / [`CrossEntropy`](CrossEntropy.md) (any number of
  classes).

None of these are `Module`s -- there is no `nn::MSE` type to construct,
no `->` indirection, no `parameters()`. Each is called directly:
`nn::losses::MSE(prediction, target)`.

## The Shared Reduction Mode

Every loss in this family takes a `reduction` field (part of that loss's
own `Options` struct) of type `nn::LossReduction`, with the same three
values and the same meaning everywhere:

- `LossReduction::Mean` (the default for every loss here) -- divide the
  sum of every element's own loss by the element count, so the result's
  scale does not depend on how many elements were involved. For the two
  losses with a `class_weight` field ([`NLL`](NLLOptions.md#class_weight),
  [`CrossEntropy`](CrossEntropyOptions.md#class_weight)), the denominator
  is instead the **sum of the gathered weights**, not the raw element
  count -- see [`NLL`](NLL.md#example-2) for the exact arithmetic this
  produces.
- `LossReduction::Sum` -- the sum alone, with no division.
- `LossReduction::None` -- no reduction at all; the per-element losses
  are returned as-is, same shape as the input (minus the class axis, for
  the multi-class classification losses).

`Mean` is the right default for training a real model: it keeps the
loss's scale (and therefore a tuned learning rate's effective step size)
independent of batch size. Reach for `Sum` only when that
batch-size-independence is specifically not wanted; reach for `None` when
the per-element values themselves are the thing being inspected, not a
single scalar summary.

## Choosing Among The Regression Losses

All five compute some function of `prediction - target`, differing only
in how aggressively a large error is penalized relative to a small one:

1. [`MSE`](MSE.md) -- squares every error. The steepest penalty for large
   errors, and the smoothest gradient near zero; the right default
   absent a specific reason to reach for something else.
2. [`RelativeMSE`](RelativeMSE.md) -- `MSE`, additionally divided by a
   caller-supplied `divisor` (typically the target's own magnitude), for
   when errors should be judged relative to the target's scale rather
   than in absolute terms.
3. [`Huber`](Huber.md) / [`SmoothL1`](SmoothL1.md) -- quadratic near
   zero, linear beyond a threshold (`delta`/`beta` respectively) --
   substantially more resistant to a single outlier dominating the loss
   than plain `MSE`, at the cost of a slightly more complex gradient.
   The two are numerically different formulas despite the similar shape
   -- see [`SmoothL1`](SmoothL1.md#example-2) for the exact numeric
   distinction; neither is simply a renamed version of the other.
4. [`L1`](L1.md) -- purely linear, everywhere. The most outlier-robust
   of the five, at the cost of a gradient that never shrinks as the
   error approaches zero. `SmoothL1` with `beta=0` reduces to this
   exactly.

## Choosing Among The Classification Losses

- [`BCEWithLogits`](BCEWithLogits.md) -- exactly two classes (or,
  equivalently, independent binary decisions per element, e.g.
  multi-label classification where each label is its own yes/no). Takes
  raw logits directly; never evaluates `sigmoid` on them, for numerical
  stability.
- [`NLL`](NLL.md) -- any number of classes, but expects
  log-probabilities as input (typically the output of a `log_softmax`
  call made separately) and only ever accepts class-index targets.
- [`CrossEntropy`](CrossEntropy.md) -- any number of classes, takes raw
  logits directly (computing `log_softmax` internally), and accepts
  *either* class-index or soft-probability targets, plus optional label
  smoothing. For the common case (class-index target, no label
  smoothing), this is exactly `NLL` composed with an internal
  `log_softmax` -- reach for `NLL` directly only when log-probabilities
  are already computed for some other reason and recomputing them would
  be wasteful.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract most of the rest of this reference implements; none of the
  losses in this family do, since they are plain functions.
- [`attention`](../attention.md) / [`rnn`](../rnn.md) -- the layer
  families whose output a loss in this family is typically called on.
