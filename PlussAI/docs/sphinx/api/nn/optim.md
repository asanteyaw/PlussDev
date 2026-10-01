# `pluss::ai::nn::optim`

`optim` owns the optimizers that step a model's parameters during
training: [`Optimizer`](optim/Optimizer.md), the base every concrete
optimizer derives from, and seven concrete algorithms --
[`SGD`](optim/SGD.md), [`Adagrad`](optim/Adagrad.md),
[`Adam`](optim/Adam.md), [`AdamW`](optim/AdamW.md),
[`NAdam`](optim/NAdam.md), [`RMSprop`](optim/RMSprop.md), and
[`Minimax`](optim/Minimax.md), a constrained primal-dual optimizer built
on top of the other six's own update rules.

Every optimizer here consumes either a [`Module`](module/Module.md)'s
own [`parameters()`](module/Module/parameters.md), or a plain list of
`Series*` -- there is no dependency in the other direction.
[`grad`](grad.md) governs whether a parameter's `forward` even builds
the graph an optimizer's `step()` would otherwise have nothing to read.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Optimizer`](optim/Optimizer.md) | the base class: `step()` (four overloads), `parameters()`, `size()`, `clear()`, per-group lane telemetry |
| [`GradientClipOptions`](optim/GradientClipOptions.md) | shared by every optimizer: clip-by-value or clip-by-norm gradient clipping |
| [`ExecutionOptions`](optim/ExecutionOptions.md) | shared by every optimizer: which update lane to run, strictness, capturable mode |
| [`StepDiagnosticsOptions`](optim/StepDiagnosticsOptions.md) | shared by every optimizer: opt-in fail-fast checks for a bad `step()` |
| [`StepLaneTelemetry`](optim/StepLaneTelemetry.md) | per-group report of which update lane ran and why |
| [`SGD`](optim/SGD.md) | stochastic gradient descent -- momentum, dampening, weight decay, Nesterov |
| [`Adagrad`](optim/Adagrad.md) | per-parameter accumulated-squared-gradient scaling |
| [`Adam`](optim/Adam.md) | adaptive moment estimation, optional AMSGrad |
| [`AdamW`](optim/AdamW.md) | `Adam` with decoupled weight decay |
| [`NAdam`](optim/NAdam.md) | `Adam` with Nesterov momentum folded into the first moment |
| [`RMSprop`](optim/RMSprop.md) | root-mean-square gradient scaling, optional centering |
| [`Minimax`](optim/Minimax.md) | a primal-dual optimizer for equality/inequality-constrained training |

## How To Call It

```cpp
namespace nn = pluss::ai::nn;

nn::Fusion model(3, 1);
nn::optim::Adam optimizer(model->parameters(), nn::optim::AdamOptions{.learning_rate = 0.01});

for (int step = 0; step < 5; ++step) {
  const pluss::Series loss = optimizer.step([&]() -> pluss::Series {
    const pluss::Series prediction = model.forward(x);
    return nn::losses::MSE(prediction, target);
  });
  std::cout << "step " << step << " loss = " << loss.item<float>() << "\n";
}
```

```text
step 0 loss = 50.8957
step 1 loss = 49.9018
step 2 loss = 48.918
step 3 loss = 47.9444
step 4 loss = 46.9812
```

Every optimizer here is constructed once, against a fixed parameter list,
and then stepped repeatedly. `step()`'s four overloads (see
[`Optimizer::step`](optim/Optimizer/step.md)) cover every way a training
loop might already be computing gradients -- a closure that runs the
forward pass and lets `step` handle backprop, explicit gradients a caller
already has, or each tracked parameter's own recorded sensitivity with no
arguments at all.

## Six Algorithms, One Shared Shape

`SGD`/`Adagrad`/`Adam`/`AdamW`/`NAdam`/`RMSprop` all share the same public
shape: three constructor overloads (parameters + options, per-group
options, or a `Module&`), an `Options` struct ending in the same two
cross-cutting fields (`clip`, `execution`), a `Group` struct for per-group
option overrides, and `options()`/`set_options()`/`set_group_options()`/
`capture_state()`/`restore_state()`. Once you know one, reading another is
mostly a matter of its own update formula and its own options fields --
each algorithm's own page focuses on exactly that difference rather than
re-explaining the shared shape.

## Related Pages

- [`module`](module.md) -- `Module::parameters()` is what most of these
  constructors consume directly.
- [`grad`](grad.md) -- the autograd toggle that governs whether there is
  anything for `step()` to read in the first place.
- [`losses`](losses.md) -- the loss functions a closure passed to
  `step()` typically calls.
- [`data`](data.md) -- `pluss::tils::DataLoader` supplies the batches a
  training loop feeds through the closure each step.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optim/Introduction
optim/Optimizer
optim/GradientClipOptions
optim/ExecutionOptions
optim/StepDiagnosticsOptions
optim/StepLaneTelemetry
optim/SGD
optim/Adagrad
optim/Adam
optim/AdamW
optim/NAdam
optim/RMSprop
optim/Minimax
```
<!-- apiref:toctree:end -->
