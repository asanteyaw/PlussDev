# `pluss::ai::ice::Callback`

The training-loop observer/hook base class -- run/epoch/batch/backward/
step/checkpoint/exception hooks, plus opt-in state persistence. Every
hook has a no-op default, so a subclass overrides only the ones it
cares about. [`Coach::learn`](../coach/Coach.md) calls every hook at a
specific, fixed point in its loop; see each hook's own page for exactly
when. [`CallbackChain`](CallbackChain.md) is the one built-in
`Callback` that fans a hook out to several others, and
[`EarlyStopCallback`](EarlyStopCallback.md) is the one built-in
`Callback` with real behavior, added automatically by `Coach` under
conditions its own page documents.

## Signature

```cpp
pluss::ai::ice::Callback::~Callback() = default;
```

An abstract-in-practice base class -- every hook has a default
implementation, so nothing is pure virtual, but a `Callback` with every
hook left at its default does nothing observable.

## Construction

A subclass constructs however it needs to (an ordinary derived-class
constructor; `Callback` itself declares none beyond the implicit
default); add an instance to a training run via
[`Coach::add_callback`](../coach/Coach.md), by `std::shared_ptr`, before
calling `Coach::learn`/`infer`/`assess`.

## State

Not applicable -- `Callback` itself holds no state. A subclass's own
state is entirely its own concern; see
[`state_key`](Callback/state_key.md)/[`save_state`](Callback/save_state.md)/
[`load_state`](Callback/load_state.md) for the opt-in mechanism to
survive a checkpoint resume.

## Methods

| Method | Fires | Covered on |
| --- | --- | --- |
| `on_run_begin(state)` | once, at the start of a run | [`on_run_begin`](Callback/on_run_begin.md) |
| `on_run_end(state)` | once, at the end of a run | [`on_run_end`](Callback/on_run_end.md) |
| `on_epoch_begin(state)` | once per epoch, `learn` only | [`on_epoch_begin`](Callback/on_epoch_begin.md) |
| `on_epoch_end(state)` | once per epoch, `learn` only | [`on_epoch_end`](Callback/on_epoch_end.md) |
| `on_batch_begin(state)` | once per batch | [`on_batch_begin`](Callback/on_batch_begin.md) |
| `on_batch_end(state)` | once per batch | [`on_batch_end`](Callback/on_batch_end.md) |
| `on_before_backward(state)` | once per training batch | [`on_before_backward`](Callback/on_before_backward.md) |
| `on_after_backward(state)` | once per training batch | [`on_after_backward`](Callback/on_after_backward.md) |
| `on_before_step(state)` | once per batch that steps | [`on_before_step`](Callback/on_before_step.md) |
| `on_checkpoint_save(state, filepath)` | once per checkpoint write | [`on_checkpoint_save`](Callback/on_checkpoint_save.md) |
| `on_checkpoint_load(state, filepath)` | once, on resume | [`on_checkpoint_load`](Callback/on_checkpoint_load.md) |
| `on_exception(state)` | once, instead of `on_run_end` | [`on_exception`](Callback/on_exception.md) |
| `state_key()` | -- (queried, not a hook) | [`state_key`](Callback/state_key.md) |
| `save_state()` | once per checkpoint write (if keyed) | [`save_state`](Callback/save_state.md) |
| `load_state(state)` | once, on resume (if keyed) | [`load_state`](Callback/load_state.md) |

## Changelog

- **0.1** -- `Callback`: the full run/epoch/batch/backward/step/
  checkpoint/exception hook surface, plus `state_key`/`save_state`/
  `load_state` for checkpoint-compatible persistence.

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
