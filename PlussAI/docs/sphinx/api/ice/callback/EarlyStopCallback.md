# `pluss::ai::ice::EarlyStopCallback`

The built-in early-stopping [`Callback`](Callback.md), driven by
[`ValidationPolicy`](../options/ValidationPolicy.md)/
[`LoopRegularization`](../options/LoopRegularization.md) -- tracks the
best value seen so far for a monitored metric and requests a stop once
`patience` validation epochs in a row fail to improve on it.

Usually never constructed directly: whenever a `Coach::learn` run has
validation enabled and either
[`LoopRegularization::early_stopping`](../options/LoopRegularization.md)
or `restore_best_weights` is `true` -- both default to `true`/`false`
respectively, so this is the common case -- `Coach` constructs one
itself and adds it to its internal
[`CallbackChain`](CallbackChain.md) automatically. Construct one
directly only to read its accessors mid-run from a caller's own code, or
to combine it with other callbacks inside a hand-built
[`CallbackChain`](CallbackChain.md).

## Signature

```cpp
pluss::ai::ice::EarlyStopCallback::EarlyStopCallback(
    pluss::ai::ice::ValidationPolicy validation, pluss::ai::ice::LoopRegularization regularization);
```

## Construction

```cpp
ice::ValidationPolicy validation{};        // monitor = "val_loss", direction = Auto
ice::LoopRegularization regularization{};  // early_stopping = true, patience = 10
ice::EarlyStopCallback early_stop(validation, regularization);
```

Both arguments are copied in, independent of whatever
`TrainOptions::validation`/`loop_regularization` a `Coach` run is using
-- a hand-constructed `EarlyStopCallback` can watch a different monitor
or patience than the run's own policy, which is exactly how to combine
one with `Coach`'s own auto-added instance without them conflicting: set
`options.loop_regularization.early_stopping = false` on the `TrainOptions`
passed to `Coach` so it does not also add its own, and add a
manually-tuned instance via
[`Coach::add_callback`](../coach/Coach.md) instead.

## State

`monitor()` / `best_value()` / `best_epoch()` / `stopped_epoch()` /
`wait_count()`
: Read-only accessors over this instance's own tracked state -- see the
  `## Methods` table below. `monitor()` is fixed at construction;
  the other four change as `on_epoch_end` fires, and reset at the start
  of every run (`on_run_begin`), except when resuming from a checkpoint
  that carried this callback's own saved state (see
  [`state_key`](Callback/state_key.md)/
  [`save_state`](Callback/save_state.md)/
  [`load_state`](Callback/load_state.md)).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `monitor()` | `const std::string&` | [`monitor`](EarlyStopCallback/monitor.md) |
| `best_value()` | `std::optional<double>` | [`best_value`](EarlyStopCallback/best_value.md) |
| `best_epoch()` | `std::int64_t` | [`best_epoch`](EarlyStopCallback/best_epoch.md) |
| `stopped_epoch()` | `std::int64_t` | [`stopped_epoch`](EarlyStopCallback/stopped_epoch.md) |
| `wait_count()` | `std::int64_t` | [`wait_count`](EarlyStopCallback/wait_count.md) |

Overrides `on_run_begin`/`on_epoch_end`/`on_run_end` (the stopping
logic) and `state_key`/`save_state`/`load_state` (checkpoint
persistence) from [`Callback`](Callback.md) -- see that type's own
method pages for what each hook means generically; this page's
`Introduction` covers the stopping algorithm these overrides implement.

## Changelog

- **0.1** -- `EarlyStopCallback`: monitor/patience/min-delta-driven
  stopping, optional best-weight restoration, and checkpoint-compatible
  state persistence.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

EarlyStopCallback/Introduction
EarlyStopCallback/best_epoch
EarlyStopCallback/best_value
EarlyStopCallback/monitor
EarlyStopCallback/stopped_epoch
EarlyStopCallback/wait_count
```
<!-- apiref:toctree:end -->
