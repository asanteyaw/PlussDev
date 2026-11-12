# `pluss::ai::nn::module`

`module` owns the contract every layer, container, and composed network in
`nn` implements — parameter/buffer/child-module registration, train/eval
mode, platform/dtype moves, and state snapshot/restore — plus the one
container every multi-layer network is actually built from.

Every other category in this reference (`activation`, `normalization`,
`fusion`, `cnn`, `rnn`, `attention`, ...) hands you concrete `Module`
subclasses. `module` is where their *shared* behavior lives, so it is the
first category to read, before any of the layer catalog.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Module`](module/Module.md) | the base class: registration, `train`/`eval`, `to`, `parameters`/`buffers`/`modules` (flat and named), `snapshot`/`restore` |
| [`Schedule`](module/Schedule.md) | a named, ordered composition of modules and callables run in sequence — the container every multi-layer network is built from |

## How Modules Become Values

Every concrete layer type you see elsewhere in this reference — `Fusion`,
`Conv1d`, `RNN`, `MultiheadAttention`, `Schedule` itself — is **not** a
`Module` subclass directly. Each is a small value-type wrapper
(`ModuleHolder<Impl>`, via the `PLUSS_MODULE_IMPL` macro) around a
`shared_ptr` to the real implementation class (`FusionImpl`, `Conv1dImpl`,
...), which *is* the `Module` subclass. For example, `Fusion` is the value
wrapper around `FusionImpl`.

`ModuleHolder` forwards every `Module` operation you need
(`train`/`eval`/`to`/`parameters`/`forward`/`operator()`/`snapshot`/`restore`)
through `operator->`, so you call `layer.forward(x)` or `layer(x)` on the
wrapper directly — you never see `FusionImpl` or write `shared_ptr` yourself.
This is why layers are copyable, default-constructible into an empty
(`nullptr`) state, and cheap to pass around: copying a `Fusion` copies a
`shared_ptr`, not the underlying weights.

## How To Call It

Put a fusion layer in evaluation mode, count its defined parameters, and roll it back to a snapshot. The program prints each check.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  nn::Fusion layer(3, 2);
  layer.train(false);
  std::cout << "training = " << (layer.is_training() ? "true" : "false") << "\n";
  int parameters = 0;
  for (pluss::Series* p : layer.parameters()) {
    if (p->defined()) ++parameters;
  }
  std::cout << "defined parameters = " << parameters << "\n";
  pluss::nn::Module::Snapshot snap = layer.snapshot();
  layer.restore(snap);
  std::cout << "restored\n";
  return 0;
}
```

Output

```text
training = false
defined parameters = 2
restored
```

## Related Pages

- [`fusion`](fusion.md), [`cnn`](cnn.md), [`rnn`](rnn.md),
  [`attention`](attention.md), [`activation`](activation.md),
  [`normalization`](normalization.md), [`embedding`](embedding.md) — the
  concrete layer types built on `Module`.
- [`optim`](optim.md) — optimizers take a `Module&` or the flat `Series*`
  list `Module::parameters()` returns.
- [`grad`](grad.md) — `GradMode`/`NoGradGuard`, the global recording toggle
  that governs whether ops on a `Module`'s parameters build a reverse-mode
  graph at all.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

module/Introduction
module/Module
module/Schedule
```
<!-- apiref:toctree:end -->
