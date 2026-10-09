# `pluss::ai::ice::Outline`

Ice's `nn::Schedule`-composition convenience: compose existing layers
into a trainable [`Model`](../model/Model.md) in one line, with no
hand-written subclass. `Outline` owns a real
[`Schedule`](../../nn/module/Schedule.md) -- composition, not
reimplementation -- so nothing about the layers themselves is
duplicated, and dropping to raw `pluss::nn::Schedule` at any point costs
nothing.

## Signature

```cpp
template <class... Layers>
explicit pluss::ai::ice::Outline::Outline(Layers&&... layers);
```

`final` -- not intended as a further base class. Every argument is
forwarded straight into the owned `Schedule`'s own constructor; see
[`Outline`'s own Introduction](Outline/Introduction.md) for the
registration step that follows.

## Construction

```cpp
ice::Outline model(nn::Fusion(2, 4), nn::Relu(), nn::Fusion(4, 1));
```

Any argument list valid for `pluss::nn::Schedule`'s own constructor is
valid here -- each becomes one auto-named stage, in order. See
[`Outline`'s own Introduction](Outline/Introduction.md) for what
happens at construction time, and [`schedule`](Outline/schedule.md) for
extending the composition afterward.

## State

`ready()` / `is_training()`
: Inherited unchanged from [`Model`](../model/Model.md); `Outline` adds
  no state of its own beyond the composed `Schedule`.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `forward(x)` | `Series` | [`forward`](Outline/forward.md) |
| `schedule()` | `Schedule&` / `const Schedule&` | [`schedule`](Outline/schedule.md) |

Every other [`Model`](../model/Model.md) method --
`compute_loss`/the four step methods/`learn`/`infer`/`assess`/`save`/
`load`/`ready`/`is_training`/`train`/`eval`/`to` -- is inherited
unchanged and documented on `Model`'s own page, not repeated here.

## Changelog

- **0.1** -- `Outline`: the variadic constructor, the `forward`
  override, and the `schedule()` accessor pair.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Outline/Introduction
Outline/forward
Outline/schedule
```
<!-- apiref:toctree:end -->
