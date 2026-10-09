# `pluss::ai::ice::Model`

The generic escape hatch: define parameters plus `forward`/`compute_loss`,
get `learn`/`infer`/`assess`/`save`/`load`, gradient clipping, and
non-finite diagnostics for free. [`Outline`](../outline/Outline.md) is
the common-case convenience built on top of this for models that are
just a chain of existing layers; `Model` is for anything else.

## Signature

```cpp
pluss::ai::ice::Model::~Model() = default;
```

An abstract base class -- `forward(const Series&)` is pure virtual,
so `Model` itself is never instantiated directly.

## Construction

A subclass constructs its own parameters/modules in its own constructor
and registers each one via the two protected methods every subclass
calls from there:

- `void register_parameter(Series& param)` -- a raw trainable `Series`
  member, by reference (non-owning).
- `void register_module(pluss::nn::Module& module)` -- an `nn::` module
  member, by reference (non-owning); every parameter that module owns is
  discovered automatically.

Both are non-owning -- the subclass's own members stay the source of
truth; `Model` just keeps pointers to them for `learn`/`infer`/`assess`/
`save`/`load`/`to` to walk. See [`Outline`](../outline/Outline.md)'s own
constructor for the simplest real example (one `register_module` call
wrapping a `Schedule`), or a hand-written subclass for one combining raw
parameters and modules together.

## State

`ready()` / `is_training()`
: Trivial `noexcept` accessors, not covered on their own detailed pages
  beyond [`ready`](Model/ready.md)/[`is_training`](Model/is_training.md):
  `ready()` is whether `learn`/`load` has been called at least once;
  `is_training()` is whether the model is in training mode (`train()`)
  or evaluation mode (`eval()`/`train(false)`).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `forward(x)` | `Series` | [`forward`](Model/forward.md) |
| `compute_loss(...)` | `Series` | [`compute_loss`](Model/compute_loss.md) |
| `learn_step(batch, ctx)` | `StepResult` | [`learn_step`](Model/learn_step.md) |
| `valid_step(batch, ctx)` | `StepResult` | [`valid_step`](Model/valid_step.md) |
| `test_step(batch, ctx)` | `StepResult` | [`test_step`](Model/test_step.md) |
| `infer_step(batch, ctx)` | `StepResult` | [`infer_step`](Model/infer_step.md) |
| `learn(...)` | `void` | [`learn`](Model/learn.md) |
| `infer(...)` | `Series` | [`infer`](Model/infer.md) |
| `assess(...)` | `AssessReport` | [`assess`](Model/assess.md) |
| `save(filepath)` | `void` | [`save`](Model/save.md) |
| `load(filepath)` | `void` | [`load`](Model/load.md) |
| `ready()` | `bool` | [`ready`](Model/ready.md) |
| `training_report()` | `const TrainingReport&` | [`training_report`](Model/training_report.md) |
| `is_training()` | `bool` | [`is_training`](Model/is_training.md) |
| `train(enabled = true)` | `void` | [`train`](Model/train.md) |
| `eval()` | `void` | [`eval`](Model/eval.md) |
| `to(...)` | `Model&` | [`to`](Model/to.md) |

## Changelog

- **0.1** — the generic `Model` base: `forward`/`compute_loss`
  extension points, the four step methods, `learn`/`infer`/`assess`
  convenience overloads, `save`/`load`, and `to`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Model/Introduction
Model/assess
Model/compute_loss
Model/eval
Model/forward
Model/infer
Model/infer_step
Model/is_training
Model/learn
Model/learn_step
Model/load
Model/ready
Model/save
Model/test_step
Model/to
Model/train
Model/training_report
Model/valid_step
```
<!-- apiref:toctree:end -->
