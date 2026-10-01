# `pluss::ai::nn::optim::GradientClipOptions`

Optional gradient clipping, applied to every parameter's gradient
independently, right after it is collected and before any optimizer-specific
formula sees it. Every concrete optimizer's own `Options` struct (
[`SGDOptions`](SGDOptions.md), [`AdamOptions`](AdamOptions.md), ...) ends
with a `clip` field of this type -- shared across all seven, so clipping is
configured identically no matter which algorithm is stepping.

Passed as part of an optimizer's own `Options` struct; the default,
`GradientClipOptions{}` (`mode = None`), disables clipping entirely.

## Fields

### `mode`

`GradientClipMode mode = GradientClipMode::None`
: Which clipping strategy to apply, if any:
  - `None` (the default) -- no clipping; the gradient passes through
    unchanged.
  - `ByValue` -- elementwise clamp: every entry is clamped into
    `[-threshold, threshold]` independently. Rejects complex gradients --
    an elementwise clamp needs a total order over the values, which
    complex numbers do not have.
  - `ByNorm` -- scales the *whole* gradient by a single factor,
    `min(1, threshold / (norm + epsilon))`, where `norm` is the
    gradient's L2 norm. Preserves direction, only shrinks magnitude, and
    only when the norm already exceeds `threshold`. Supports complex
    gradients (the norm is always a real, non-negative scalar, so no
    ordering over complex values is needed).

### `threshold`

`double threshold = 0.0`
: The clipping bound `ByValue`/`ByNorm` clamp or scale against. Must be
  `> 0.0` and finite when `mode != None` -- construction (well, the owning
  optimizer's construction) throws otherwise. Ignored when `mode == None`.

### `epsilon`

`double epsilon = 1e-12`
: A small constant added to the norm's denominator in `ByNorm` mode, to
  avoid dividing by (near) zero when a gradient's norm is already tiny.
  Must be `> 0.0` and finite when `mode != None`. Has no effect in
  `ByValue` mode or when `mode == None`.

## Guidance

Reach for `ByValue` when individual gradient entries occasionally spike to
extreme values and the fix should be "clamp each one independently" --
simple, but it does not preserve the gradient's overall direction, since
different entries can be clamped by different amounts. Reach for `ByNorm`
(the more common choice in practice, and the one that matches how most
published training recipes describe "gradient clipping") when the goal is
to cap the *overall* update magnitude while keeping every entry's relative
contribution intact -- the whole gradient is scaled by one factor, never
reshaped. Leave `mode` at `None` (the default) unless a training run is
actually showing the instability clipping is meant to guard against;
clipping every step has a real cost and changes the effective step size
even when nothing would have exploded.

## See Also

- [`ExecutionOptions`](ExecutionOptions.md) -- the other field every
  concrete optimizer's `Options` ends with.
- [`Optimizer::step`](Optimizer/step.md) -- clipping runs as part of every
  overload, regardless of where the gradient came from.
