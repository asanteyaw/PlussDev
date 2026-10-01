# `pluss::ai::nn::optim::Optimizer` Introduction

`Optimizer` owns the parameter list an optimizer steps and the machinery
every concrete algorithm shares: collecting gradients (either explicitly
supplied or read off each parameter's own recorded sensitivity), clipping
them per [`GradientClipOptions`](../GradientClipOptions.md), resolving
which update lane actually runs per
[`ExecutionOptions`](../ExecutionOptions.md), running the opt-in
[`StepDiagnosticsOptions`](../StepDiagnosticsOptions.md) checks, and
recording [`StepLaneTelemetry`](../StepLaneTelemetry.md) for the step just
taken. What it does *not* own is the actual update formula -- computing
the next parameter value from a gradient and whatever per-parameter state
that formula needs (`apply_step_`) is a pure virtual method, implemented
once per concrete algorithm.

This split is why `Optimizer` itself cannot be constructed: there is no
default answer to "what is the next parameter value", only seven concrete
answers ([`SGD`](../SGD.md), [`Adagrad`](../Adagrad.md),
[`Adam`](../Adam.md), [`AdamW`](../AdamW.md), [`NAdam`](../NAdam.md),
[`RMSprop`](../RMSprop.md), [`Minimax`](../Minimax.md)). Everything else --
both constructors, and every method on this page -- is inherited by all
seven unchanged, which is why this page's examples construct a concrete
subclass (most use `SGD`, the simplest) to demonstrate behavior that
applies identically to every one of them.

## Reading Order

1. [`step`](step.md) -- the core method, and the one with the most surface:
   four overloads covering every way a training loop might already be
   producing gradients.
2. [`parameters`](parameters.md) / [`size`](size.md) -- read-only
   introspection over the parameter list this optimizer was constructed
   with.
3. [`clear`](clear.md) -- resets tracked parameters' recorded sensitivity;
   relevant mainly for the no-arg `step()` overload's manual-tape usage
   pattern.
4. [`last_step_lane_telemetry`](last_step_lane_telemetry.md) -- inspect
   which update lane actually ran on the most recent `step()` call, and
   why, when [`ExecutionOptions`](../ExecutionOptions.md)`.lane` was left
   at `Auto` or a requested lane fell back.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="clear.html">
    <span class="label">Next</span>
    <span class="title">clear</span>
  </a>
</div>
<!-- apiref:nav:end -->
