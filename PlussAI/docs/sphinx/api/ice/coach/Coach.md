# `pluss::ai::ice::Coach`

An alternative, model-external entry point for `learn`/`infer`/`assess`
orchestration -- the same operations
[`Model`](../model/Model.md)'s own convenience methods run internally,
callable against any `Model` plus explicit
[`Callback`](../callback/Callback.md)s. `Model::learn` always
constructs a fresh `Coach` under the hood. It installs configured built-ins but
cannot receive caller-supplied callbacks; construct one
directly whenever a run needs its own callbacks, or when the same
[`TrainOptions`](../options/TrainOptions.md) should drive several calls
against different models without repeating it.

## Signature

```cpp
explicit pluss::ai::ice::Coach::Coach(
    pluss::ai::ice::TrainOptions options = {},
    std::vector<std::shared_ptr<pluss::ai::ice::Callback>> callbacks = {});
```

## Construction

```cpp
ice::Coach defaulted;                                        // default TrainOptions, no callbacks
ice::Coach with_options(options);                             // explicit options, no callbacks
ice::Coach with_both(options, {logger, early_stop});          // both up front
```

Both constructor arguments default, so a bare `Coach{}` is valid --
equivalent to `Model::learn`'s own internal construction, minus the
callbacks a caller adds afterward via
[`add_callback`](Coach/add_callback.md).

## State

`options()`
: The `TrainOptions` this instance runs every call under -- see
  [`options`](Coach/options.md).

`callbacks()`
: The callbacks added so far, in order -- see
  [`callbacks`](Coach/callbacks.md).

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `options()` | `const TrainOptions&` | [`options`](Coach/options.md) |
| `callbacks()` | `const std::vector<std::shared_ptr<Callback>>&` | [`callbacks`](Coach/callbacks.md) |
| `add_callback(callback)` | `Coach&` | [`add_callback`](Coach/add_callback.md) |
| `learn(...)` (4 overloads) | `void` | [`learn`](Coach/learn.md) |
| `infer(...)` (3 overloads) | `Series` | [`infer`](Coach/infer.md) |
| `assess(...)` (3 overloads) | `AssessReport` | [`assess`](Coach/assess.md) |

## Changelog

- **0.1** -- `Coach`: the model-external `learn`/`infer`/`assess`
  orchestration surface, with `options`/`callbacks`/`add_callback`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Coach/Introduction
Coach/add_callback
Coach/assess
Coach/callbacks
Coach/infer
Coach/learn
Coach/options
```
<!-- apiref:toctree:end -->
