# `pluss::ai::nn::activation`

`activation` owns the elementwise nonlinearities every network is built
from -- the layers that sit between [`fusion`](fusion.md)/[`cnn`](cnn.md)/
[`rnn`](rnn.md) stages so a stack of linear operations does not collapse
into one big linear operation. Every type here is a
[`Module`](module/Module.md), constructible with no arguments (or, for most
of them, one small options struct). All of them add no parameters to a
network's [`parameters()`](module/Module/parameters.md) except
[`PReLU`](activation/PReLU.md), whose negative-side slope is itself a
learnable weight.

## The Surface

| Entity | Shape | What it gives you |
| --- | --- | --- |
| [`ReLU`](activation/ReLU.md) | `max(0, x)` | the default hidden-layer activation |
| [`Sigmoid`](activation/Sigmoid.md) | `(0, 1)` | a probability-shaped squash, `0.5`-centered |
| [`Tanh`](activation/Tanh.md) | `(-1, 1)` | the same squash, `0`-centered |
| [`Softplus`](activation/Softplus.md) | smooth `ReLU` | differentiable everywhere, tunable sharpness |
| [`LeakyReLU`](activation/LeakyReLU.md) | `ReLU` + a leak | keeps a small gradient for negative inputs |
| [`Elu`](activation/Elu.md) | smooth `ReLU` + a leak | a saturating (not linear) negative side |
| [`SiLU`](activation/SiLU.md) | `x * sigmoid(x)` | smooth, non-monotonic, common in modern architectures |
| [`GELU`](activation/GELU.md) | `x * Phi(x)` | the standard transformer feed-forward activation |
| [`Hardshrink`](activation/Hardshrink.md) | `x` or `0` | zeroes a band around `0`, passes everything else through unchanged |
| [`Softmax`](activation/Softmax.md) | sums to `1` | turns logits into a per-axis probability distribution |
| [`LogSoftmax`](activation/LogSoftmax.md) | `log` of `Softmax` | the numerically stable form losses like cross-entropy expect |
| [`PReLU`](activation/PReLU.md) | `ReLU` + a learned leak | `LeakyReLU`'s slope, trained instead of fixed |
| [`Identity`](activation/Identity.md) | `x` | a structural no-op, for conditional architectures |

## How To Call It

Build a two-layer network with a ReLU between the layers, then run a batch of two rows through it. The program prints the logit shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  nn::Schedule net;
  net.append("fc1", nn::Fusion(784, 128));
  net.append("act1", nn::ReLU());
  net.append("fc2", nn::Fusion(128, 10));
  const pluss::Series x = pluss::make_series(std::vector<double>(2 * 784, 0.5), {2, 784});
  const pluss::Series logits = net.forward(x);
  std::cout << "logits = " << logits.sizes()[0] << " x " << logits.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
logits = 2 x 10
```

Every type here is added into a [`Schedule`](module/Schedule.md) the same
way any other layer is -- there is nothing activation-specific about
`append`/`insert`. The only structural difference from `fusion`/`cnn`/`rnn` is
that none of these types register any parameters, so a `Schedule`'s
[`parameters()`](module/Module/parameters.md) is unaffected by how many
activation stages it has.

## Related Pages

- [`module`](module.md) -- the `Module` contract every type here implements,
  and `Schedule`, the container they compose into.
- [`functional`](functional.md) -- `Elu` also has a stateless
  [`elu`](functional/elu.md) function form, for callers that want the
  computation without constructing a `Module`.
- [`losses`](losses.md) -- `BCEWithLogits` computes a numerically stable
  fused form of `Sigmoid` + binary cross-entropy, rather than composing
  `Sigmoid` with a separate loss call.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

activation/Introduction
activation/ReLU
activation/Sigmoid
activation/Tanh
activation/Softplus
activation/LeakyReLU
activation/Elu
activation/Identity
activation/SiLU
activation/GELU
activation/Hardshrink
activation/Softmax
activation/LogSoftmax
activation/PReLU
```
<!-- apiref:toctree:end -->
