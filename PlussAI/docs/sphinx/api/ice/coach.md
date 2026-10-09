# `pluss::ai::ice::coach`

`coach` owns [`Coach`](coach/Coach.md), the model-external `learn`/
`infer`/`assess` orchestration surface -- the same operations
[`Model`](model.md)'s own convenience methods run internally, callable
against any `Model` with explicit
[`Callback`](callback/Callback.md)s attached.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Coach`](coach/Coach.md) | `options()`/`callbacks()`/`add_callback(...)` plus `learn`/`infer`/`assess`, each with a `Source&` primitive overload and `Series`-pair/`DataFrame` convenience overloads built on it |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

class LogLossCallback final : public ice::Callback {
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
coach.add_callback(std::make_shared<LogLossCallback>());
coach.learn(model, x, y);
```

```text
epoch 0: loss = 2.35997
```

Reach for `Coach` directly instead of `Model::learn` whenever a run
needs its own callbacks -- `Model::learn` always constructs a fresh,
callback-less `Coach` internally and has no way to attach one. The same
`Coach` instance can also drive several `learn`/`infer`/`assess` calls
(against the same or different models) without repeating
`TrainOptions`.

## Related Pages

- [`callback`](callback.md) -- `Callback`/`RunState`/`CallbackChain`/
  `EarlyStopCallback`, everything a `Coach`-driven run threads through.
- [`source`](source.md) -- `Source`/`TensorSource`/`DataSetSource`,
  what every `learn`/`infer`/`assess` overload ultimately runs through.
- [`model`](model.md) -- `Model::learn`, the convenience wrapper built
  on top of this.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

coach/Introduction
coach/Coach
```
<!-- apiref:toctree:end -->
