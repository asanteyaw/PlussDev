# `pluss::ai::nn::functional`

`functional` owns the stateless free-function surface -- exactly what
every stateful `Module` class elsewhere in this reference calls
internally, exposed directly for callers who want the computation
without constructing a `Module`. Also reachable via the short
`pluss::nn::*` alias (see [`Introduction`](functional/Introduction.md)).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`fusion`](functional/fusion.md) | the stateless fully connected op |
| [`dropout`](functional/dropout.md) | stateless elementwise dropout |
| [`elu`](functional/elu.md) | stateless ELU |
| [`conv`](functional/conv.md) | stateless N-D convolution (`conv`/`conv2d`/`conv3d`) |
| [`conv1d`](functional/conv1d.md) | stateless 1-D convolution |
| [`causal_padding1d`](functional/causal_padding1d.md) | the left-only padding that makes a 1-D convolution causal |
| [`rnn`](functional/rnn.md) / [`gru`](functional/gru.md) / [`lstm`](functional/lstm.md) | stateless recurrent forward over explicit weights |
| [`scaled_attention`](functional/scaled_attention.md) | stateless scaled dot-product attention |
| [`multihead_attention`](functional/multihead_attention.md) | stateless multi-head attention over explicit projection weights |
| [`causal_attention_mask`](functional/causal_attention_mask.md) / [`square_subsequent_mask`](functional/square_subsequent_mask.md) | build attention masks that block future positions |
| [`NetWeight`](functional/NetWeight.md) / [`NetConfig`](functional/NetConfig.md) | the explicit weight/configuration types `rnn`/`gru`/`lstm` take |
| [`MultiheadWeights`](functional/MultiheadWeights.md) | the explicit projection weight/bias bundle `multihead_attention` takes |
| [`AttentionOptions`](functional/AttentionOptions.md) / [`MultiheadOptions`](functional/MultiheadOptions.md) | the options structs for the two attention functions |

## How To Call It

Apply a fused linear map with an explicit weight matrix, then run a one-step recurrent pass with explicit parameters. The program prints both output shapes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {1, 3});
  const pluss::Series weight = pluss::make_series({1.0, 0.0, 0.0, 0.0, 1.0, 0.0}, {2, 3});
  const pluss::Series y = nn::fusion(x, weight);
  std::cout << "fusion = " << y.sizes()[0] << " x " << y.sizes()[1] << "\n";
  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.5}, {1, 1});
  params.weight_hh = pluss::make_series({0.0}, {1, 1});
  params.bias = pluss::make_series({0.0}, {1});
  const pluss::Series seq = pluss::make_series({1.0}, {1, 1, 1});
  const auto [output, hidden] = nn::rnn(seq, params);
  std::cout << "rnn output = " << output.sizes()[0] << " x " << output.sizes()[1] << " x " << output.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
fusion = 1 x 2
rnn output = 1 x 1 x 1
```

Every function here is a plain function -- no `Module` to construct, no
`->` indirection, no `parameters()`. Weights that a `Module` class would
otherwise own and register are supplied directly as arguments.

## Related Pages

- [`module`](module.md) -- the `Module` contract every function here
  powers a stateful counterpart of.
- [`fusion`](fusion.md) / [`cnn`](cnn.md) / [`rnn`](rnn.md) /
  [`attention`](attention.md) -- the stateful `Module` families whose
  `forward` methods call these functions internally.
- [`losses`](losses.md) -- the other family of plain (stateless)
  functions in this reference, for contrast.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

functional/Introduction
functional/fusion
functional/dropout
functional/elu
functional/conv
functional/conv1d
functional/rnn
functional/gru
functional/lstm
functional/scaled_attention
functional/multihead_attention
functional/causal_attention_mask
functional/square_subsequent_mask
functional/causal_padding1d
functional/AttentionOptions
functional/MultiheadOptions
functional/MultiheadWeights
functional/NetWeight
functional/NetConfig
```
<!-- apiref:toctree:end -->
