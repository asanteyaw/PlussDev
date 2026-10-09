# `pluss::ai::nn::embedding`

`embedding` owns the layer that turns a categorical integer code into a
dense, learned vector -- the standard bridge between raw category data
(a product ID, a token, a calendar field) and the continuous
[`activation`](activation.md)/[`fusion`](fusion.md) layers that follow it.
Right now that's a single type, [`Embedding`](embedding/Embedding.md) -- a
[`Module`](module/Module.md) like everything else in this reference, and,
like [`LayerNorm`](normalization/LayerNorm.md), it carries learnable state
and requires an explicit options argument at construction, since there is
no meaningful default table shape.

## The Surface

| Entity | Shape | What it gives you |
| --- | --- | --- |
| [`Embedding`](embedding/Embedding.md) | `index` -> `index` + `hidden_size` | turns integer category codes into dense learned vectors |

## How To Call It

Look up three category codes in a 1000-entry embedding table of 16-dimensional vectors, then apply a ReLU. The program prints the output shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  nn::EmbeddingOptions opts{};
  opts.cardinality = 1000;
  opts.hidden_size = 16;
  nn::Schedule net;
  net.append("emb", nn::Embedding(opts));
  net.append("act1", nn::ReLU());
  const pluss::Series index = pluss::make_series({0LL, 7LL, 999LL}, {3}).to(pluss::ScalarType::Int64);
  const pluss::Series y = net.forward(index);
  std::cout << "embedding = " << y.sizes()[0] << " x " << y.sizes()[1] << "\n";
  return 0;
}
```

Output

```text
embedding = 3 x 16
```

`Embedding` composes into a [`Schedule`](module/Schedule.md) the same way
any other layer does -- it just expects an integer-valued `index` `Series`
as its input rather than a `Series` of features, so it is almost always
the first stage in a chain, not somewhere in the middle.

## Related Pages

- [`module`](module.md) -- the `Module` contract `Embedding` implements,
  and `Schedule`, the container it composes into.
- [`normalization`](normalization.md) -- the other family with a
  required-options, no-default-constructor layer, for comparison.
- [`activation`](activation.md) -- the elementwise nonlinearities
  typically placed right after an `Embedding` lookup (or after the
  `Fusion` that follows it).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

embedding/Introduction
embedding/Embedding
```
<!-- apiref:toctree:end -->
