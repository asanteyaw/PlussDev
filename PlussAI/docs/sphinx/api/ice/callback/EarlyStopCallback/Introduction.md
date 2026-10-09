# `pluss::ai::ice::EarlyStopCallback` Introduction

## The Stopping Algorithm

`on_epoch_end` is where everything happens, and only during the `Learn`
stage, and only once `state.epoch >= validation.start_from_epoch`:

1. Read the monitored metric off `RunState` via
   [`RunState::metric(validation.monitor)`](../RunState/metric.md) --
   throws if it was not produced this epoch (for example, monitoring
   `"val_accuracy"` when nothing ever calls
   [`put_metric`](../RunState/put_metric.md) with that name).
2. Compare it against the best value seen so far, using
   [`ValidationPolicy::direction`](../../options/ValidationPolicy.md)
   (resolved from the metric's own name when `Auto`) and
   [`LoopRegularization::min_delta`](../../options/LoopRegularization.md)
   as the minimum change that counts as an improvement.
3. If it improved: record the new best value/epoch, reset
   [`wait_count`](wait_count.md) to `0`, and -- when
   `restore_best_weights` is set -- snapshot every parameter in
   `RunState::parameters` right now.
4. If it did not improve: increment `wait_count`; once `wait_count >=
   patience` and `early_stopping` is set, request a stop
   (`RunState::stop_requested = true`, `stop_reason = "early_stopping"`)
   and record `stopped_epoch`.

`on_run_end` then restores parameters, but only when
`restore_best_weights` is set: from the in-memory snapshot if one was
taken, or (when resuming a run with no in-memory snapshot carried over)
by loading `RunState::best_checkpoint_path` through
[`Model::load`](../../model/Model/load.md) instead.

## `early_stopping` Versus `restore_best_weights`

These two fields are independent, and `EarlyStopCallback` is added
automatically whenever *either* is set, not only for `early_stopping`:

- `early_stopping = true, restore_best_weights = false` -- the common
  case. Training can end before `max_epochs`, and the final-epoch
  weights are kept regardless of whether that epoch was the best one.
- `early_stopping = false, restore_best_weights = true` -- training
  always runs the full `max_epochs`, but ends with the best epoch's
  parameters rather than the last epoch's. `wait_count`/`best_value`/
  `best_epoch` are still tracked throughout (see
  [`LoopRegularization::early_stopping`](../../options/LoopRegularization.md)),
  they just never trigger a stop.
- Both `true` -- the usual pairing: stop early, and land on the best
  weights either way.

## Checkpoint Interaction

`state_key()` returns `"early_stop"`; `save_state`/`load_state`
round-trip every field this type tracks (`monitor`, `direction`,
`start_from_epoch`, `early_stopping`, `patience`, `min_delta`,
`restore_best_weights`, `best_value`, `best_epoch`, `stopped_epoch`,
`wait_count`) as a flat string map -- the same shape
[`Callback::save_state`](../Callback/save_state.md)'s own page
documents generically. `Coach::learn` walks every callback's `state_key`/`save_state` into a
checkpoint, and applies every callback's `load_state` back out when
resuming -- but only *after* its own
[`on_run_begin`](../Callback/on_run_begin.md) has already fired (see
[`io::load_checkpoint`'s own page](../../io/load_checkpoint.md) for
why: `on_run_begin` always resets `wait_count`/`best_value`/`best_epoch`
to their defaults unconditionally, so restoring any earlier would just
be overwritten). The net effect is still correct -- by the time the
resumed run's first epoch begins, those fields hold the checkpoint's
real values, restored one hook after `on_run_begin`'s own reset ran
and was immediately superseded. The in-memory parameter snapshot is
*not* carried across a checkpoint resume either way -- only
`best_checkpoint_path`-based restoration works after resuming, not the
in-memory snapshot path.

## Related Pages

- [`Callback`](../Callback.md) -- the base class; `on_run_begin`/
  `on_epoch_end`/`on_run_end`/`state_key`/`save_state`/`load_state` are
  all overridden here.
- [`CallbackChain`](../CallbackChain.md) -- what `Coach` assembles this
  into alongside any manually added callbacks.
- [`ValidationPolicy`](../../options/ValidationPolicy.md) /
  [`LoopRegularization`](../../options/LoopRegularization.md) -- every
  field this type reads.
- [`Coach`](../../coach/Coach.md) -- adds this automatically under the
  conditions described on `EarlyStopCallback`'s own page.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="best_epoch.html">
    <span class="label">Next</span>
    <span class="title">best_epoch</span>
  </a>
</div>
<!-- apiref:nav:end -->
