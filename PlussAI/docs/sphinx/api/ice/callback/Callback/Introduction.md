# `pluss::ai::ice::Callback` Introduction

## What `Callback` Is

`Callback` is `ice`'s training-loop observer: a base class with one
no-op virtual method per point in [`Coach`](../../coach/Coach.md)'s
`learn`/`infer`/`assess` loop where something might want to react --
log a metric, checkpoint, stop early, inspect a gradient. A subclass
overrides only the hooks it needs; every other hook stays a no-op.
Instances are added to a run via
[`Coach::add_callback`](../../coach/Coach.md), by `std::shared_ptr`
(shared ownership -- the same instance can be reused across runs, or
held onto after a run to read whatever state it accumulated).

## Hook Firing Order (`Coach::learn`)

```text
on_run_begin
  [on_checkpoint_load]                 -- only when resuming
  for each epoch:
    on_epoch_begin
    for each training batch:
      on_batch_begin
      on_before_backward
      on_after_backward
      [on_before_step]                 -- only on a batch that steps
      on_batch_end
    for each validation batch (if this epoch validates):
      on_batch_begin
      on_batch_end
    on_epoch_end
    [on_checkpoint_save]                -- once per save_best/save_last write
on_run_end                              -- or on_exception, not both
```

`Coach::infer`/`assess` run a much shorter version of the same shape:
`on_run_begin`, then `on_batch_begin`/`on_batch_end` per batch (no
epochs, no backward/step, no checkpointing), then `on_run_end` (or
`on_exception`).

## `state.stop_requested` And Where It Actually Cuts Off

Every hook except `on_run_begin`/`on_run_end`/`on_exception`/
`on_checkpoint_save`/`on_checkpoint_load` is immediately followed by a
check of `state.stop_requested`, but *where* that check breaks out of
differs by hook:

- From [`on_epoch_begin`](../Callback/on_epoch_begin.md): breaks the
  epoch loop immediately -- that epoch's batches, validation, and
  `on_epoch_end` never run.
- From [`on_batch_begin`](../Callback/on_batch_begin.md)/
  [`on_before_backward`](../Callback/on_before_backward.md)/
  [`on_after_backward`](../Callback/on_after_backward.md)/
  [`on_before_step`](../Callback/on_before_step.md): breaks the
  *current* batch loop (training or validation, whichever is running)
  -- but the epoch's other loop (validation, if the stop happened during
  training) and `on_epoch_end` still run, since neither is gated on
  `state.stop_requested`. Only the *next* epoch is actually skipped.
- From [`on_batch_end`](../Callback/on_batch_end.md)/
  [`on_epoch_end`](../Callback/on_epoch_end.md): the current unit of
  work (batch or epoch) already finished normally; only what comes
  after is skipped.

See each hook's own page for a verified example of its specific cutoff
point.

## Related Pages

- [`RunState`](../RunState.md) -- what every hook reads and writes.
- [`CallbackChain`](../CallbackChain.md) -- fans one hook call out to
  several callbacks, in the order they were added.
- [`EarlyStopCallback`](../EarlyStopCallback.md) -- the one built-in
  callback with real behavior, built entirely on this hook surface.
- [`Coach`](../../coach/Coach.md) -- drives every hook call described
  here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="load_state.html">
    <span class="label">Next</span>
    <span class="title">load_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
