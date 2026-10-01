# `pluss::ai::nn::optim::Optimizer`

`Optimizer` is the base class every concrete optimizer in this reference
([`SGD`](SGD.md), [`Adagrad`](Adagrad.md), [`Adam`](Adam.md),
[`AdamW`](AdamW.md), [`NAdam`](NAdam.md), [`RMSprop`](RMSprop.md),
[`Minimax`](Minimax.md)) derives from. It owns the parameter list an
optimizer steps, the shared `step()` machinery (gradient collection,
clipping dispatch, execution-lane resolution, diagnostics), and the update
lane telemetry every step produces -- everything about *running* an
optimizer that does not depend on which algorithm's own formula computes
the actual parameter delta.

`Optimizer` is not directly constructible: its update rule
(`apply_step_`) is a pure virtual method with no implementation of its own,
so only a concrete subclass can be instantiated. Every method on this page
is nonetheless part of every concrete optimizer's own public surface
unchanged -- read this page once, and `step`/`parameters`/`size`/`clear`/
`last_step_lane_telemetry` work identically no matter which algorithm you
construct.

## Signature

```cpp
explicit nn::optim::Optimizer::Optimizer(pluss::RangeView<pluss::Series*> parameters);
explicit nn::optim::Optimizer::Optimizer(nn::Module& module);

virtual nn::optim::Optimizer::~Optimizer() = default;
```

Both constructors are inherited unchanged by every concrete optimizer, each
of which also adds its own third overload taking that algorithm's own
`Options` (for example `SGD(RangeView<Series*> parameters, SGDOptions
options = {})`) -- see each algorithm's own page for that third form.

## Construction

```cpp
// explicit parameter list -- pluss::RangeView<Series*> accepts a
// std::vector<Series*> directly, no wrapping needed
std::vector<Series*> params = {&w, &b};
nn::optim::SGD opt_a(params);

// from a Module -- collects Module::parameters() (every registered
// parameter, recursively through every registered child) automatically.
// A concrete layer like Fusion is a ModuleHolder wrapper, not a Module
// itself, so dereference it (*model) to reach the Module& overload.
nn::Fusion model(3, 1);
nn::optim::SGD opt_b(*model);
```

Both forms populate the same underlying parameter list; the `Module&`
overload exists purely to save a caller from writing
`model->parameters()` themselves. Every parameter must be a *defined*
`Series` of a floating or complex dtype -- construction throws otherwise.
A parameter that is not yet marked to track its slope
(`tracks_slope()`) is switched on automatically during construction, since
an optimizer with nothing to eventually receive a gradient for would be
useless.

## State

`parameters()` -- `const vector<Series*>&`
: The flat parameter list this optimizer was constructed with, in
  construction order. See [`parameters`](Optimizer/parameters.md).

`size()` -- `size_t`
: `parameters().size()`. See [`size`](Optimizer/size.md).

`last_step_lane_telemetry()` -- `const vector<StepLaneTelemetry>&`
: One entry per parameter group from the most recent `step()` call. See
  [`last_step_lane_telemetry`](Optimizer/last_step_lane_telemetry.md).

`step_diagnostics()` / `set_step_diagnostics(options)`
: The [`StepDiagnosticsOptions`](StepDiagnosticsOptions.md) currently
  active on this optimizer -- a plain get/set pair, not covered on its own
  page since, like [`Module::is_training`](../module/Module/train.md),
  it is a trivial one-line accessor around a stored value. Defaults to
  `StepDiagnosticsOptions{}` (every check off) until set explicitly.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `step()` / `step(gradients)` / `step(closure)` (two closure shapes) | `void` / `void` / `Series` | [`step`](Optimizer/step.md) |
| `parameters()` | `const vector<Series*>&` | [`parameters`](Optimizer/parameters.md) |
| `size()` | `size_t` | [`size`](Optimizer/size.md) |
| `clear()` | `void` | [`clear`](Optimizer/clear.md) |
| `last_step_lane_telemetry()` | `const vector<StepLaneTelemetry>&` | [`last_step_lane_telemetry`](Optimizer/last_step_lane_telemetry.md) |

## Changelog

- **0.1** — the shared base: construction from a parameter list or a
  `Module&`, `step`'s four overloads, and the rest of the contract every
  concrete optimizer inherits.

## See Also

- [`GradientClipOptions`](GradientClipOptions.md) /
  [`ExecutionOptions`](ExecutionOptions.md) /
  [`StepDiagnosticsOptions`](StepDiagnosticsOptions.md) -- the three
  options structs `step()`'s behavior is configured through.
- [`SGD`](SGD.md), [`Adam`](Adam.md), and the other five concrete
  algorithms -- each adds its own `Options`, its own `apply_step_`
  formula, and its own per-parameter state on top of this contract.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Optimizer/Introduction
Optimizer/clear
Optimizer/last_step_lane_telemetry
Optimizer/parameters
Optimizer/size
Optimizer/step
ExecutionOptions
GradientClipOptions
StepDiagnosticsOptions
```
<!-- apiref:toctree:end -->
