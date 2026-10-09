# `pluss::ai::ice::RunState`

Mutable run-wide state a [`Callback`](Callback.md)'s hooks read and
write: model pointer, stage, platform, epoch/step counters, a named
metrics map, checkpoint paths, and stop-request/exception bookkeeping.
[`Coach`](../coach/Coach.md) owns the one `RunState` instance for a
given `learn`/`infer`/`assess` run and passes it by reference to every
hook call in order -- a callback that writes `state.stop_requested`,
calls `state.put_metric(...)`, or reads `state.model` is observing and
steering the same object `Coach` itself is driving the loop from.

## Signature

```cpp
pluss::ai::ice::RunState::RunState() = default;

[[nodiscard]] std::optional<double> pluss::ai::ice::RunState::metric(std::string_view name) const;
void pluss::ai::ice::RunState::put_metric(std::string name, double value);
```

A plain aggregate with public fields -- see `## State` below for every
field. `metric`/`put_metric` are the only member functions; everything
else is read or written directly.

## Construction

Never constructed by a `Callback` author directly -- [`Coach`](../coach/Coach.md)
builds the one `RunState` for a run (`Coach::learn`/`infer`/`assess`
each build their own) and passes it by reference into every hook. A
`Callback` implementation only ever receives one as a `RunState&`
parameter.

## State

### `model`

`Model* model = nullptr`
: The model being trained/inferred/assessed, or `nullptr` outside a
  run. [`EarlyStopCallback`](EarlyStopCallback.md) reads this to call
  [`Model::load`](../model/Model/load.md) when restoring the best
  checkpoint and no in-memory parameter snapshot was kept.

### `stage`

`StepStage stage = StepStage::Infer`
: Which phase is active right now: `Learn`, `Valid`, `Test`, or
  `Infer` -- see [`StepContext::stage`](../step/StepContext.md) for the
  same enum's full meaning. During `Coach::learn`, this toggles
  between `Learn` and `Valid` batch-by-batch as the validation pass
  runs inside the same epoch; `on_epoch_end` always sees it back at
  `Learn`.

### `target_platform`

`Platform target_platform = pluss::pCPU`
: The platform [`resolve_platform`](../platform/resolve_platform.md)
  resolved this run onto, set once at the start of the run.

### `epoch`

`std::int64_t epoch = -1`
: The current epoch index (`0`-based), or `-1` before the first epoch
  begins. Unchanged during a `Coach::infer`/`assess` run (no epochs).

### `batch_index`

`std::int64_t batch_index = -1`
: The current batch's index within its stage (`0`-based, restarting at
  `0` for the validation pass), or `-1` between batches.

### `global_step`

`std::int64_t global_step = 0`
: A monotonically increasing counter, incremented once per optimizer
  step across the whole run (not reset per epoch) -- unaffected by
  `accumulation_steps` batches that do not trigger a step.

### `rows`

`std::int64_t rows = 0`
: How many rows the current batch holds. Set before `on_batch_begin`
  and left at that value through `on_batch_end`.

### `metrics`

`std::unordered_map<std::string, double> metrics = {}`
: The current batch or epoch's named metrics -- `"loss"` always,
  `"val_loss"` once validation has run. Cleared at the start of every
  batch and at the start of every epoch; read/write it through
  [`metric`](RunState/metric.md)/[`put_metric`](RunState/put_metric.md)
  rather than the map directly, so a missing key reads as
  `std::nullopt` instead of default-constructing an entry.

### `parameters`

`const std::vector<Series*>* parameters = nullptr`
: Pointer to the model's registered parameters for this run, or
  `nullptr` outside `Coach::learn`.
  [`EarlyStopCallback`](EarlyStopCallback.md) uses this to snapshot and
  restore the best weights in memory when
  `LoopRegularization::restore_best_weights` is set.

### `recipe`

`const TrainOptions* recipe = nullptr`
: Pointer to the `TrainOptions` driving this run, or `nullptr` outside
  `Coach::learn`. Lets a callback read any option -- validation policy,
  checkpoint policy, execution policy -- without the `Coach` call site
  threading it through separately.

### `stop_requested`

`bool stop_requested = false`
: Set this to `true` from any hook to end the run early, checked by
  `Coach::learn` immediately after every hook call. See
  [`on_epoch_end`](Callback/on_epoch_end.md) for the built-in
  [`EarlyStopCallback`](EarlyStopCallback.md)'s own use of this field.

### `stop_reason`

`std::string stop_reason = {}`
: A short human-readable reason, set alongside `stop_requested` --
  `EarlyStopCallback` sets `"early_stopping"`. Persisted into a
  checkpoint's own state if one is saved after the run ends.

### `exception_message`

`std::string exception_message = {}`
: Set to `e.what()` just before
  [`on_exception`](Callback/on_exception.md) fires, empty otherwise.

### `checkpoint_path`

`std::string checkpoint_path = {}`
: The path of whichever checkpoint was most recently saved or loaded,
  updated right before
  [`on_checkpoint_save`](Callback/on_checkpoint_save.md)/
  [`on_checkpoint_load`](Callback/on_checkpoint_load.md) fires.

### `best_checkpoint_path` / `last_checkpoint_path`

`std::string best_checkpoint_path = {}`, `std::string last_checkpoint_path = {}`
: The paths of the best-so-far and most-recent checkpoints specifically,
  independent of `checkpoint_path` -- populated only when
  `CheckpointPolicy::save_best`/`save_last` are enabled.

### `best_metric` / `best_metric_name`

`std::optional<double> best_metric = std::nullopt`, `std::string best_metric_name = {}`
: The best value seen so far for the monitored metric
  (`ValidationPolicy::monitor`) and that metric's own name, maintained
  by `Coach::learn` itself for `checkpoint.save_best` -- independent of
  whatever an `EarlyStopCallback` tracks internally for early stopping.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `metric(name)` | `std::optional<double>` | [`metric`](RunState/metric.md) |
| `put_metric(name, value)` | `void` | [`put_metric`](RunState/put_metric.md) |

## Changelog

- **0.1** -- `RunState`: every field `Coach` threads through a run, plus
  the `metric`/`put_metric` accessor pair.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

RunState/Introduction
RunState/metric
RunState/put_metric
```
<!-- apiref:toctree:end -->
