# `pluss::ai::nn::optim::Optimizer`

The base class every optimizer derives from -- owns the parameter list and drives step() from either explicit gradients or each parameter's own recorded sensitivity.

## Signature

```cpp
// the public factories / constructors that produce this object
// e.g. lymph::models::Model lymph::models::logistic_regression(...);
```

## Construction

<!-- how an instance is obtained; delete if the Signature block says it all -->

## State

<!-- the observable fields / accessors and what each means -->

## Methods

<!-- one line per method, linking its page; the toctree below carries the sidebar entries -->

## Changelog

- **0.1** — <!-- initial documented surface -->

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
