# `pluss::ai::nn::fusion`

`fusion` owns the fully connected (dense/linear) layer -- the workhorse
every network in this reference is at least partly built from, and the
layer every worked example in [`module`](module.md) and
[`activation`](activation.md) has already used to demonstrate
[`Schedule`](module/Schedule.md). Right now that's a single type,
[`Fusion`](fusion/Fusion.md) -- a [`Module`](module/Module.md) like
everything else in this reference, carrying two learnable parameters
(`weight` and, optionally, `bias`).

## The Surface

| Entity | Shape | What it gives you |
| --- | --- | --- |
| [`Fusion`](fusion/Fusion.md) | `[..., in_features]` -> `[..., out_features]` | the learned affine map `y = x @ weight^T + bias` |

## How To Call It

Build a two-layer network whose second layer has no bias, then run two rows through it. The program prints the output shape.

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
  net.append("fc2", nn::Fusion(128, 10, /*bias=*/false));
  const pluss::Series x = pluss::make_series(std::vector<double>(2 * 784, 0.5), {2, 784});
  const pluss::Series y = net.forward(x);
  std::cout << "output = " << y.sizes()[0] << " x " << y.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
output = 2 x 10
```

`Fusion` composes into a [`Schedule`](module/Schedule.md) the same way any
other layer does -- it just happens to be the layer nearly every chain in
this reference includes at least once, either as the first stage (turning
raw features into a hidden representation) or the last (turning a hidden
representation into a prediction).

## Related Pages

- [`module`](module.md) -- the `Module` contract `Fusion` implements, and
  `Schedule`, the container it composes into.
- [`activation`](activation.md) -- the elementwise nonlinearities
  conventionally placed right after a `Fusion` stage.
- [`embedding`](embedding.md) -- the other parameterized layer with the
  same uniform-bound initialization strategy, for a categorical rather
  than continuous input.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

fusion/Introduction
fusion/Fusion
```
<!-- apiref:toctree:end -->
