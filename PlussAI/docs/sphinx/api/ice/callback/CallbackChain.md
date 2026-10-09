# `pluss::ai::ice::CallbackChain`

A [`Callback`](Callback.md) that fans every hook out to an ordered list
of other callbacks. `CallbackChain` *is* a `Callback` -- every hook it
overrides calls the same hook, in order, on each callback it holds --
so one chain can stand in anywhere a single `Callback` is expected,
including as [`Coach::add_callback`](../coach/Coach.md)'s own argument.
This is exactly how [`Coach`](../coach/Coach.md) itself works
internally: the callbacks added via `Coach::add_callback`, plus a
built-in [`EarlyStopCallback`](EarlyStopCallback.md) when validation and
early stopping/`restore_best_weights` are both active, are assembled
into one `CallbackChain` at the start of every `learn` call.

## Signature

```cpp
pluss::ai::ice::CallbackChain::CallbackChain() = default;
explicit pluss::ai::ice::CallbackChain::CallbackChain(std::vector<std::shared_ptr<Callback>> callbacks);
```

Inherits every `Callback` hook, overriding each one to fan out across
`callbacks()` -- see `## Methods` below.

## Construction

```cpp
ice::CallbackChain empty;                                              // no callbacks yet
ice::CallbackChain seeded({logger, early_stop});                       // two callbacks up front
```

Build empty and grow with [`add`](CallbackChain/add.md), or construct
directly from a `std::vector<std::shared_ptr<Callback>>`. Both shapes
are equivalent -- the constructor is exactly `add`'s bulk form.

## State

`callbacks()`
: The ordered list this chain fans hooks out to -- see
  [`callbacks`](CallbackChain/callbacks.md).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `add(callback)` | `void` | [`add`](CallbackChain/add.md) |
| `callbacks()` | `const std::vector<std::shared_ptr<Callback>>&` | [`callbacks`](CallbackChain/callbacks.md) |

Every `Callback` hook (`on_run_begin`, `on_run_end`, `on_epoch_begin`,
`on_epoch_end`, `on_batch_begin`, `on_batch_end`, `on_before_backward`,
`on_after_backward`, `on_before_step`, `on_checkpoint_save`,
`on_checkpoint_load`, `on_exception`) is also overridden, each simply
forwarding to the same hook on every held callback, in `callbacks()`
order -- not separately documented here; see
[`Callback`](Callback.md)'s own method pages for what each hook means
and when it fires. `state_key`/`save_state`/`load_state` are **not**
overridden -- a `CallbackChain` has no state of its own to persist, and
`Coach::learn`'s checkpoint logic walks `callbacks()` directly for each
held callback's own state rather than going through the chain's.

## Changelog

- **0.1** -- `CallbackChain`: the fan-out `Callback` implementation,
  `add`/`callbacks`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

CallbackChain/Introduction
CallbackChain/add
CallbackChain/callbacks
```
<!-- apiref:toctree:end -->
