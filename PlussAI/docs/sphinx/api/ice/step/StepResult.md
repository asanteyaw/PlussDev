# `pluss::ai::ice::StepResult`

The predictions and/or loss a [`Model`](../model/Model.md) step method
returns -- either may be left undefined depending on the stage.
[`infer_step`](../model/Model.md) never sets `loss`;
[`learn_step`](../model/Model.md)/[`valid_step`](../model/Model.md)/
[`test_step`](../model/Model.md) set both.

## Signature

```cpp
pluss::ai::ice::StepResult::StepResult() = default;
```

## Construction

Default-constructed, both fields undefined -- then set directly (it is
a plain aggregate: `StepResult{.predictions = p, .loss = l}`) by a
[`Model`](../model/Model.md) override's step method. Most callers never
construct one by hand; it comes back from calling `learn_step`/
`valid_step`/`test_step`/`infer_step` directly.

## State

`predictions` / `loss`
: Both `Series`, possibly undefined. `predictions` is the forward pass's
  output; `loss` is `compute_loss`'s result, left undefined by
  `infer_step` (there is no target to compute a loss against during
  plain inference). Check [`has_predictions`](StepResult/has_predictions.md)/
  [`has_loss`](StepResult/has_loss.md) rather than assuming either is
  set.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `has_predictions()` | `bool` | [`has_predictions`](StepResult/has_predictions.md) |
| `has_loss()` | `bool` | [`has_loss`](StepResult/has_loss.md) |

## Changelog

- **0.1** — `predictions`/`loss`, the result of every `Model` step
  method.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

StepResult/Introduction
StepResult/has_loss
StepResult/has_predictions
```
<!-- apiref:toctree:end -->
