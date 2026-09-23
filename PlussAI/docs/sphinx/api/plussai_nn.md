# PlussAI `nn`

`nn` is PlussAI's neural-network module framework: the layer, container,
loss, and optimizer building blocks for constructing and training deep models
by hand, plus a lightweight stateless functional surface for calling any of
those operations directly.

Unlike `classic`, `nn` is not organized by learning-task family -- there is
no single natural grouping like "estimators" or "prep" for a neural-network
engine. Instead this reference follows the category shape every neural-network
reference actually converges on: a base module contract, layer categories
(activation, normalization, embedding, the fully connected layer, convolution,
recurrent, attention), loss functions, a parallel functional surface, an
autograd toggle, optimizers, and the data-loading utilities purpose-built to
feed them.

## Public Positioning

Reach for `nn` when you want to construct a model's forward pass yourself:

- compose layers (`Fusion`, `Conv1d`, `RNN`, `MultiheadAttention`, ...) into a
  `Schedule`
- call the same operations statelessly, with explicit weights, via the
  functional surface
- compute a loss and step an optimizer directly against `Module::parameters()`
- load and batch training data with `pluss::loader::DataLoader`

Reach for [`ice`](plussai_ice.md) when you want those same layers and
optimizers behind a higher-level training/runtime loop -- fit/predict-style
entry points, callbacks, checkpointing -- built around `nn` rather than
replacing it.

## Include Surface

```cpp
#include <pluss/ai/nn.h>   // module classes: Module, Schedule, Fusion, Conv1d, RNN, ...
#include <pluss/nn.h>      // the above, plus the short pluss::nn::* alias and the
                            // functional surface (pluss::nn::fusion, pluss::nn::conv, ...)
```

## Public Namespace

```cpp
namespace nn = pluss::ai::nn;
```

`nn` was already a clean, human-presentable namespace at the source level --
unlike `nodej`/`nodeb`/`nodek`, which are internal source-directory codenames
that public docs never expose (translated instead into `classic`, the `nlp`
substrate, and so on), `pluss::ai::nn` needs no separate public alias to hide
anything, the same way `torch::nn` needs none. Prose and signatures on this
reference spell the framework out in full (`pluss::ai::nn::Fusion`,
`pluss::ai::nn::optim::Adam`) for clarity. Runnable examples declare the short
`nn` alias and call through it (`nn::Fusion`, `nn::optim::Adam`), the same way
the codebase does. The functional surface and the `losses`/`optim`
sub-namespaces are shown at their full path where an example needs them.

Everything under `pluss::ai::nn` is also reachable through the shorter,
pre-existing top-level alias `pluss::nn::*` (`pluss::nn::Fusion`,
`pluss::nn::fusion`, `pluss::nn::optim::Adam`) -- both spellings name the same
namespace, and `pluss::nn` is in fact how most of the existing codebase
already writes it. This reference standardizes on the fully qualified
`pluss::ai::nn` in prose and signatures, matching how `classic`, `nlp`, and
`lymph` are each spelled at full depth.

## Relationship To Other Frameworks

- [`ice`](plussai_ice.md) is the training/runtime framework built around
  `nn` -- it does not duplicate layers or optimizers, it drives them.
- `nn` does not depend on [`classic`](plussai_classic.md); the two engines
  are independent.
- [`procedure`](plussai_procedure.md) can persist and route `nn` artifacts
  (trained `Module` state) inside larger execution graphs.

The separation of responsibility:

- `nn` answers "how do I build and train a neural network's forward pass,
  exactly?"
- `ice` answers "how do I run a training loop over one cleanly, with
  checkpointing and callbacks?"
- `procedure` answers "how do I persist and replay a trained model as an
  artifact?"

## Categories

This reference follows the framework -> category -> type narrowing order.
Each category page states what the concern owns, what it does not, and the
first type or function to reach for.

```{toctree}
:maxdepth: 3

nn/module
nn/activation
nn/normalization
nn/embedding
nn/fusion
nn/cnn
nn/rnn
nn/attention
nn/losses
nn/functional
nn/grad
nn/optim
nn/data
```

## Reading Strategy

1. [module](nn/module.md) -- the `Module` base contract every layer and
   container derives from, and `Schedule`, the container that composes them.
2. [activation](nn/activation.md), [normalization](nn/normalization.md),
   [embedding](nn/embedding.md), [fusion](nn/fusion.md),
   [cnn](nn/cnn.md), [rnn](nn/rnn.md), and [attention](nn/attention.md)
   -- the layer catalog, reached for as a model's architecture needs them.
3. [losses](nn/losses.md) -- the stateless loss functions a training loop
   calls against a model's output.
4. [functional](nn/functional.md) -- the same operations as (2), called
   statelessly over explicit weights instead of through a `Module`.
5. [grad](nn/grad.md) -- the autograd recording toggle (`GradMode`,
   `NoGradGuard`), reached for around inference or evaluation code.
6. [optim](nn/optim.md) -- the optimizers that step a model's parameters
   during training.
7. [data](nn/data.md) -- `DataLoader` and the train/val/test and
   walk-forward splitting utilities purpose-built to feed a training loop.

## Documentation Standard

This handwritten branch is the public `nn` reference. Each page carries full
prose on every overload, parameter, shape rule, and failure mode, plus enough
verified end-to-end examples that a reader finishes it with no gap about
another way to make the same call.
