# `pluss::ai::ice::Callback`

The training-loop observer/hook base class -- run/epoch/batch/backward/step/checkpoint/exception hooks, plus opt-in state persistence.

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

Callback/Introduction
Callback/load_state
Callback/on_after_backward
Callback/on_batch_begin
Callback/on_batch_end
Callback/on_before_backward
Callback/on_before_step
Callback/on_checkpoint_load
Callback/on_checkpoint_save
Callback/on_epoch_begin
Callback/on_epoch_end
Callback/on_exception
Callback/on_run_begin
Callback/on_run_end
Callback/save_state
Callback/state_key
```
<!-- apiref:toctree:end -->
