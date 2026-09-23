# `pluss::ai::nn::Module` Introduction

## Role

`Module` is the base every layer, container, and composed network in `nn`
derives from — directly (`FusionImpl`, `Conv1dImpl`, `ScheduleImpl`, ...) or
indirectly, through `detail::RNNBase<Options>` for the recurrent layers. You
rarely name `Module` in your own code: every concrete layer type
(`Fusion`, `Conv1d`, `RNN`, `Schedule`, ...) is a small `ModuleHolder`-wrapped
value type that forwards this page's whole method set through `operator->`
(see the category's own [Introduction](../Introduction.md) for exactly how
that wrapping works). Every method on this page and its children applies
identically whether you call it on a bare `Module&` or on any wrapped layer
value.

## Lifecycle

A `Module` starts in training mode (`is_training()` is `true` unless
constructed with `Module(false)`). Its state moves through three concerns,
independently:

1. **Registration** — a subclass's constructor calls the protected
   `parameter_record`/`buffer_record`/`module_record` (see the category
   [Introduction](../Introduction.md)) to build up `params_`/`buffers_`/
   `children_`. This happens once, at construction, and is not part of this
   page's public surface.
2. **Mode and placement** — [`train`](train.md)/[`eval`](eval.md) flip
   `is_training()` and propagate to every registered child; [`to`](to.md)
   moves every registered parameter and buffer (and every child's, and
   theirs) to a new platform and/or dtype in place.
3. **Introspection and checkpointing** — [`parameters`](parameters.md) /
   [`named_parameters`](named_parameters.md) /
   [`buffers`](buffers.md) / [`named_buffers`](named_buffers.md) /
   [`modules`](modules.md) / [`named_modules`](named_modules.md) walk the
   registration tree; [`snapshot`](snapshot.md) / [`restore`](restore.md)
   capture and reapply a deep copy of every named parameter and buffer's
   values, plus the training flag.

## Related Pages

- [`module` Introduction](../Introduction.md) — the `ModuleHolder` wrapping
  mechanism and the registration discipline every subclass follows.
- [`Schedule`](../Schedule.md) — the container built on this base.
- [`optim::Optimizer`](../../optim/Optimizer.md) — constructed from a
  `Module&`, reading `parameters()` to know what to step.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="buffers.html">
    <span class="label">Next</span>
    <span class="title">buffers</span>
  </a>
</div>
<!-- apiref:nav:end -->
