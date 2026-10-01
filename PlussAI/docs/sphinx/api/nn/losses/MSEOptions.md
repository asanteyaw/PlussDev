# `pluss::ai::nn::losses::MSEOptions`

The reduction mode.

Passed to [`MSE`](MSE.md)'s third argument, defaulted -- `MSE(prediction,
target)` alone is equivalent to `MSE(prediction, target, {})`.

## Fields

### `reduction`

`nn::LossReduction reduction = nn::LossReduction::Mean`
: How the per-element squared errors are combined into the returned
  value. `Mean` (the default) divides the sum by the total element
  count; `Sum` skips that division; `None` returns the per-element
  errors unreduced, same shape as `prediction`. See
  [`Introduction`](Introduction.md#the-shared-reduction-mode) for the
  full three-way explanation shared by every loss in this family.

## Guidance

`Mean` is the right default for training a real network -- it keeps the
loss's scale independent of batch size, so a learning rate tuned for one
batch size stays sensible at another. Reach for `Sum` when the scale
should track the element count deliberately (rare), or `None` when the
per-element errors themselves are the thing being inspected, not a
single scalar loss.

## See Also

- [`MSE`](MSE.md) -- the only consumer of this options struct.
- [`Introduction`](Introduction.md) -- the shared `reduction` semantics
  every loss in this family uses identically.
