# `pluss::ai::nn::attention`

`attention` owns the attention-based layers: learned multi-head
scaled dot-product attention ([`MultiheadAttention`](attention/MultiheadAttention.md)),
and the full Transformer-family blocks/stacks/model built on top of it
([`TransformerEncoderLayer`](attention/TransformerEncoderLayer.md)/
[`TransformerEncoder`](attention/TransformerEncoder.md),
[`TransformerDecoderLayer`](attention/TransformerDecoderLayer.md)/
[`TransformerDecoder`](attention/TransformerDecoder.md), and
[`Transformer`](attention/Transformer.md), the composition of both
stacks). Unlike [`rnn`](rnn.md), which processes a sequence one timestep
at a time, every layer here sees its whole input sequence at once and
lets every position attend directly to every other position.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`MultiheadAttention`](attention/MultiheadAttention.md) | learned multi-head scaled dot-product attention over query/key/value |
| [`TransformerEncoderLayer`](attention/TransformerEncoderLayer.md) | one self-attention + feedforward encoder block |
| [`TransformerEncoder`](attention/TransformerEncoder.md) | a stack of `TransformerEncoderLayer` blocks |
| [`TransformerDecoderLayer`](attention/TransformerDecoderLayer.md) | one self-attention + cross-attention + feedforward decoder block |
| [`TransformerDecoder`](attention/TransformerDecoder.md) | a stack of `TransformerDecoderLayer` blocks |
| [`Transformer`](attention/Transformer.md) | the full encoder-decoder model, composed from the two stacks above |

## How To Call It

Run self-attention on one token, then run a transformer encoder layer on the same input. The program prints the output width of each.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/nn.h>
#include <pluss/nn.h>

#include <iostream>
#include <tuple>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  nn::MultiheadAttention attn(8, 2);
  const pluss::Series x = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, {1, 1, 8});
  const auto [output, weights] = attn.forward(x, x, x);
  std::cout << "attention output = " << output.sizes()[2] << "\n";
  nn::TransformerEncoderLayer layer(8, 2, /*dim_feedforward=*/16);
  const pluss::Series y = layer.forward(x);
  nn::Schedule net;
  net.append(layer);
  std::cout << "encoder output = " << y.sizes()[2] << "\n";
  return 0;
}
```

Output

```text
attention output = 8
encoder output = 8
```

`MultiheadAttention::forward` always returns a `tuple<Series, Series>`
(output and attention weights together, with no toggle to skip the
weights) -- adding one to a [`Schedule`](module/Schedule.md) needs
[`Schedule::append_as`](module/Schedule/append_as.md), the same as any
`rnn` layer. Every Transformer-family block/stack above it, by contrast,
returns a plain `Series` from `forward` -- ordinary `append` works
directly, as shown above.

## Related Pages

- [`module`](module.md) -- the `Module` contract every type here
  implements, and `Schedule`, the container they compose into.
- [`rnn`](rnn.md) -- the sequence-at-a-time alternative, for contrast.
- [`fusion`](fusion.md) -- every Transformer-family block's own
  feedforward sub-network is built from
  [`Fusion`](fusion/Fusion.md) layers directly.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

attention/Introduction
attention/MultiheadAttention
attention/TransformerEncoderLayer
attention/TransformerEncoder
attention/TransformerDecoderLayer
attention/TransformerDecoder
attention/Transformer
```
<!-- apiref:toctree:end -->
