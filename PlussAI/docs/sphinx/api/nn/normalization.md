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

```cpp
namespace nn = pluss::ai::nn;

nn::LayerNormOptions opts{};
opts.normalized_size = 128;  // must match the width of the axis being normalized

nn::Schedule net;
net.append("fc1", nn::Fusion(784, 128));
net.append("norm1", nn::LayerNorm(opts));
net.append("act1", nn::ReLU());

const Series y = net.forward(x);
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
