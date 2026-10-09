# `pluss::ai::ice::Outline` Introduction

## What `Outline` Is

`Outline` is a [`Model`](../../model/Model.md) subclass that owns a
[`Schedule`](../../../nn/module/Schedule.md) -- `pluss::ai::nn`'s named,
ordered composition of layers -- and does nothing else. It overrides
exactly one method, [`forward`](forward.md), which runs the input
through the composed schedule and returns the last stage's output.
Everything else `Model` provides (`compute_loss`, the four step
methods, `learn`/`infer`/`assess`, `save`/`load`, `to`) is inherited
unchanged, because none of it needs to know whether the concrete
`forward` implementation is a hand-written subclass or a plain chain of
existing layers.

This is the one-liner path: `ice::Outline model(nn::Fusion(2, 4),
nn::Relu(), nn::Fusion(4, 1));` is a complete, trainable `Model` with no
further code. A hand-written `Model` subclass is only needed when the
architecture is not a straight chain -- branching, multiple inputs, a
custom loss, or per-stage logic beyond what `Schedule`'s own
`append_as<In, Out>` input/output typing can express.

## Construction

`Outline`'s constructor is a variadic template forwarding every
argument straight into the owned `Schedule`'s own constructor:

```cpp
template <class... Layers>
explicit Outline(Layers&&... layers);
```

Each argument becomes one auto-named stage, in the order given --
identical to constructing a `Schedule` directly with the same arguments.
The constructor then calls `register_module` once, on the whole
`Schedule` object, which is how every layer's parameters -- present at
construction time or [appended later](schedule.md) -- are discovered
for [`learn`](../../model/Model/learn.md)/
[`save`](../../model/Model/save.md)/
[`to`](../../model/Model/to.md) without any further registration code.

## State

`Outline` adds no state of its own beyond the inherited
[`ready()`](../../model/Model/ready.md)/
[`is_training()`](../../model/Model/is_training.md) flags -- see
[`schedule()`](schedule.md) for the one piece of `Outline`-specific
state, the composed `Schedule` itself.

## Why The Using Declaration

`Outline` overrides only the `Series` overload of
[`Model::forward`](../../model/Model/forward.md); the base class also
declares a virtual `Batch` overload (default: `forward(batch.inputs())`).
In plain C++, a derived class that overrides one overload of a name
hides every other overload of that name at the derived class's own
scope -- not a virtual-dispatch rule, a name-lookup rule. Without a
corrective declaration, `outline_instance.forward(some_batch)` would
fail to compile on a concrete `Outline`, even though the inherited
`Batch` overload is still there, still virtual, and still reachable
through a `Model&`/`Model*` reference to the same object.

`Outline` adds `using Model::forward;` right above its own override to
bring the hidden overload back into scope, so both forms work directly
on a concrete `Outline` with no base-class reference needed. Any
hand-written `Model` subclass that overrides only one overload of
`forward` needs the same `using`-declaration for the same reason -- see
[`Model::forward`'s own page](../../model/Model/forward.md) for a second,
independent example of exactly this fix on a hand-written subclass.

## Related Pages

- [`Model`](../../model/Model.md) -- the base class `Outline` derives
  from; every method not listed on `Outline`'s own page is inherited
  from there unchanged.
- [`Schedule`](../../../nn/module/Schedule.md) -- the composition
  `Outline` owns and runs through on every `forward` call.
- [`coach`](../../coach.md) -- the orchestration `learn`/`infer`/
  `assess` delegate to, for `Outline` exactly as for any other `Model`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="forward.html">
    <span class="label">Next</span>
    <span class="title">forward</span>
  </a>
</div>
<!-- apiref:nav:end -->
