# `ice::callback` Introduction

## What This Category Owns

`callback` owns the full training-loop observer surface:
[`Callback`](Callback.md) (the hook base class every observer derives
from), [`RunState`](RunState.md) (the one mutable object every hook
call shares), [`CallbackChain`](CallbackChain.md) (a `Callback` that
fans out to several others), and
[`EarlyStopCallback`](EarlyStopCallback.md) (early stopping and best-weight
restoration), plus [`EpochLoggerCallback`](EpochLoggerCallback.md) (live
epoch metrics and terminal progress rendering).

## How The Types Relate

[`Coach`](../coach/Coach.md) holds a flat list of
`std::shared_ptr<Callback>` (added one at a time via
[`add_callback`](../coach/Coach.md)) and, at the start of every `learn`
call, assembles them -- plus an auto-added
[`EarlyStopCallback`](EarlyStopCallback.md), when conditions its own
page documents are met, and an auto-added
[`EpochLoggerCallback`](EpochLoggerCallback.md) when
`TrainOptions::progress.verbose` is enabled -- into one
[`CallbackChain`](CallbackChain.md). That chain is itself a `Callback`,
so `Coach`'s own internal hook calls (`callbacks.on_run_begin(state)`,
and so on) are just ordinary `Callback` calls that happen to fan out.
Every hook call anywhere in the loop passes the same single
[`RunState`](RunState.md) instance, by reference, for the whole run.

## Reading Order

1. [`RunState`](RunState.md) -- what every hook reads and writes; worth
   understanding first since every other page assumes it.
2. [`Callback`](Callback.md) -- the hook surface itself, and its own
   [Introduction](Callback/Introduction.md) for the full firing order
   and the `stop_requested` cutoff semantics, hook by hook.
3. [`CallbackChain`](CallbackChain.md) -- how more than one `Callback`
   runs in a single `learn` call.
4. [`EarlyStopCallback`](EarlyStopCallback.md) -- monitored stopping and
   best-weight restoration.
5. [`EpochLoggerCallback`](EpochLoggerCallback.md) -- live epoch reporting,
   cadence, metric selection, and custom output streams.

## Related Pages

- [`coach`](../coach.md) -- `Coach`, what drives every hook call.
- [`model`](../model.md) -- `Model::learn`, which installs configured built-ins
  but does not take caller-supplied callbacks directly.
- [`options`](../options.md) -- `TrainOptions`, including
  `ValidationPolicy`/`LoopRegularization`/`CheckpointPolicy`, all read
  by the callback surface.
