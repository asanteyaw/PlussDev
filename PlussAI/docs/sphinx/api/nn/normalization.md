# `pluss::ai::nn::normalization`

`normalization` owns layers that rescale their input's statistics rather
than reshape its values pointwise the way [`activation`](activation.md)
does. Right now that's a single type, [`LayerNorm`](normalization/LayerNorm.md)
-- it is a [`Module`](module/Module.md) like everything else in this
reference, but unlike most of `activation` it carries two learnable
parameters (`weight` and `bias`) and requires an explicit options argument
at construction, since there is no meaningful default width to normalize
over.

## The Surface

| Entity | Shape | What it gives you |
| --- | --- | --- |
| [`LayerNorm`](normalization/LayerNorm.md) | mean `0`, var `1` per row | stabilizes each row's scale, then a learned affine restores expressiveness |

## How To Call It

Build a layer that maps 784 inputs to 128 features, then layer-normalize and apply a ReLU. The input is float32 so it matches the default layer parameters. The program prints the output shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  nn::LayerNormOptions opts{};
  opts.normalized_size = 128;
  nn::Schedule net;
  net.append("fc1", nn::Fusion(784, 128));
  net.append("norm1", nn::LayerNorm(opts));
  net.append("act1", nn::ReLU());
  const pluss::Series x = pluss::make_series(std::vector<float>(2 * 784, 0.5F), {2, 784});
  const pluss::Series y = net.forward(x);
  std::cout << "output = " << y.sizes()[0] << " x " << y.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
output = 2 x 128
```

`LayerNorm` composes into a [`Schedule`](module/Schedule.md) the same way
any other layer does. Unlike [`activation`](activation.md)'s types, it adds
two parameters (`weight`, `bias`) to a `Schedule`'s
[`parameters()`](module/Module/parameters.md) -- the same way
[`Fusion`](fusion.md) or any other trainable layer does.

## Related Pages

- [`module`](module.md) -- the `Module` contract `LayerNorm` implements,
  and `Schedule`, the container it composes into.
- [`activation`](activation.md) -- the elementwise nonlinearities
  `LayerNorm` is typically placed alongside, most often right before one.
- [`activation/PReLU`](activation/PReLU.md) -- the only other parameterized
  layer documented so far, for comparison of how learnable state is
  exposed.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

normalization/Introduction
normalization/LayerNorm
```
<!-- apiref:toctree:end -->
