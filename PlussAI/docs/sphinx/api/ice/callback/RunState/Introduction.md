# `pluss::ai::ice::RunState` Introduction

## What `RunState` Is

`RunState` is the one piece of mutable, shared state every
[`Callback`](../Callback.md) hook sees -- [`Coach`](../../coach/Coach.md)
constructs exactly one per `learn`/`infer`/`assess` call and threads it,
by reference, through every hook call in order. There is no per-callback
copy: when a [`CallbackChain`](../CallbackChain.md) fans one hook out to
several callbacks, every one of them reads and writes the *same*
`RunState` object, in the order they were added -- a field one callback
writes is visible to the next one in the chain, for that same hook call
and every later one.

## Lifecycle

A `RunState` is built fresh at the start of every `Coach::learn`/
`infer`/`assess` call, with `model`/`parameters`/`recipe` already
pointing at the run's real objects and every counter at its starting
sentinel (`epoch = -1`, `batch_index = -1`, `global_step = 0`). It is
never reused across two separate `learn` calls, even against the same
`Coach` instance -- each call builds its own.

Most fields are owned and written by `Coach` itself (`stage`, `epoch`,
`batch_index`, `rows`, the checkpoint paths, `best_metric`/
`best_metric_name`) and should be treated as read-only from a
`Callback`'s own hooks. Three fields exist specifically for callbacks to
write: [`metrics`](../RunState.md#metrics) (through
[`metric`](metric.md)/[`put_metric`](put_metric.md)), and
`stop_requested`/`stop_reason`, which a callback sets directly to end
the run early -- see
[`EarlyStopCallback::on_epoch_end`](../Callback/on_epoch_end.md) for the
built-in example.

## Related Pages

- [`Callback`](../Callback.md) -- every hook this state is passed into.
- [`CallbackChain`](../CallbackChain.md) -- fans one `RunState` out to
  several callbacks in order.
- [`Coach`](../../coach/Coach.md) -- builds and owns the one `RunState`
  per run.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="metric.html">
    <span class="label">Next</span>
    <span class="title">metric</span>
  </a>
</div>
<!-- apiref:nav:end -->
