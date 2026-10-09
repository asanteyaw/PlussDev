# `pluss::ai::ice::callback`

`callback` owns the training-loop observer surface:
[`Callback`](callback/Callback.md) (the hook base class),
[`RunState`](callback/RunState.md) (the shared state every hook reads
and writes), [`CallbackChain`](callback/CallbackChain.md) (fans one
hook out to several callbacks),
[`EarlyStopCallback`](callback/EarlyStopCallback.md) (early stopping and
best-weight restoration), and
[`EpochLoggerCallback`](callback/EpochLoggerCallback.md) (live epoch metrics
and terminal progress bars).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Callback`](callback/Callback.md) | the hook base class -- 12 lifecycle hooks plus 3 checkpoint-persistence methods, every one a no-op by default |
| [`RunState`](callback/RunState.md) | the one mutable object every hook call shares -- model/stage/epoch/metrics/stop-request bookkeeping |
| [`CallbackChain`](callback/CallbackChain.md) | a `Callback` that fans every hook out to an ordered list of other callbacks |
| [`EarlyStopCallback`](callback/EarlyStopCallback.md) | the built-in early-stopping/best-weight-restoration callback, added automatically under common conditions |
| [`EpochLoggerCallback`](callback/EpochLoggerCallback.md) | the built-in live reporter, added automatically when `TrainOptions::progress.verbose` is enabled |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

class PrintLossCallback final : public ice::Callback {
 public:
  void on_epoch_end(ice::RunState& state) override {
    std::cout << "epoch " << state.epoch << ": loss = " << state.metric("loss").value_or(-1.0) << "\n";
  }
};

ice::Outline model(nn::Fusion(2, 1));
const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

ice::TrainOptions options{};
options.execution.max_epochs = 1;
options.validation.split_ratio = 0.0;

ice::Coach coach(options);
coach.add_callback(std::make_shared<PrintLossCallback>());
coach.learn(model, x, y);
```

```text
epoch 0: loss = 2.35997
```

For ordinary epoch output, configure `TrainOptions::progress`; both
`Model::learn` and `Coach::learn` use it. Reach for a custom `Callback` for anything that needs to observe or
steer a run from the inside -- logging, custom metrics, gradient
inspection, a non-standard stopping rule -- rather than wrapping
`Model::learn`/`Coach::learn` itself. `Model::learn`'s own convenience
overloads run through a fresh `Coach` and can install policy-driven built-ins,
but they cannot receive caller-supplied callbacks; use
[`Coach`](coach.md) directly (`Coach(options).add_callback(...).learn(...)`)
to add one.

## Related Pages

- [`coach`](coach.md) -- `Coach`, what drives every hook call.
- [`model`](model.md) -- `Model::learn`, which installs configured built-ins
  but does not take caller-supplied callbacks directly.
- [`options`](options.md) -- `TrainOptions`, including
  `ValidationPolicy`/`LoopRegularization`/`CheckpointPolicy`, all read
  by the callback surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

callback/Introduction
callback/RunState
callback/Callback
callback/CallbackChain
callback/EarlyStopCallback
callback/EpochLoggerCallback
```
<!-- apiref:toctree:end -->
