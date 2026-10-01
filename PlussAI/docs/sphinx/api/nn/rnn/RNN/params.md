<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e018b63094dc -->

# `pluss::ai::nn::RNN::params`

[Back to `pluss::ai::nn::RNN`](../RNN.md)

Only reachable via `->` (`net->params()`).

## Signature

```cpp
std::vector<pluss::ai::rnn::NetWeight>& nn::RNN::params();
const std::vector<pluss::ai::rnn::NetWeight>& nn::RNN::params() const;
```

## What It Does

Exposes the raw per-`(layer, direction)` weight blocks
[`forward`](forward.md) reads from directly -- the same registered
`Series` objects `parameters()`/`named_parameters()` (inherited from
`Module`) also see, just grouped by block instead of flattened. Reach for
`params()` when a specific block's `weight_ih`/`weight_hh`/bias needs to
be inspected or set by hand (as every example on this page does); reach
for `named_parameters()` when what matters is the dotted name each tensor
was registered under.

## Parameters

None.

## Returns

A `std::vector<pluss::ai::rnn::NetWeight>&` (or `const`), one entry per
`(layer, direction)` pair, ordered layer-major then direction-minor:
layer 0's forward-direction block, then layer 0's backward-direction
block if `options().bidirectional`, then layer 1's, and so on. Each
`NetWeight` has:

- `weight_ih` -- `[gate_size, layer_input_size]`, the input-to-hidden
  projection. `gate_size` is `hidden_size` for `RNN` (one projection per
  step); `layer_input_size` is `options().input_size` for layer 0's
  blocks, `hidden_size * dirs` for every layer above.
- `weight_hh` -- `[gate_size, hidden_size]`, the hidden-to-hidden
  projection.
- `bias` -- `[gate_size]`, defined when `options().bias == true` and
  `options().split_bias == false`; undefined otherwise.
- `bias_ih` / `bias_hh` -- `[gate_size]` each, defined when
  `options().bias == true` and `options().split_bias == true`; undefined
  otherwise.
- `weight_hr` -- always undefined for `RNN` (this field exists on
  `NetWeight` only for [`LSTM`](../LSTM.md)'s optional projection).

## Errors

None.

## Practical Examples

Three examples: reading a fresh instance's block count and shapes, setting
a block's weights by hand (the same pattern every `RNN::forward` example
on this page's sibling uses), and confirming the `const` overload sees the
same data.

### Example 1

A 2-layer, unidirectional `RNN` has exactly 2 weight blocks, with layer 1's
`weight_ih` fed by layer 0's `hidden_size`-wide output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNNOptions opts{};
  opts.input_size = 3;
  opts.hidden_size = 5;
  opts.num_layers = 2;
  nn::RNN net(opts);

  std::cout << "params().size()=" << net->params().size() << "\n";
  std::cout << "layer 0 weight_ih shape: [" << net->params()[0].weight_ih.sizes()[0] << ", "
            << net->params()[0].weight_ih.sizes()[1] << "] -- [hidden_size, input_size]\n";
  std::cout << "layer 1 weight_ih shape: [" << net->params()[1].weight_ih.sizes()[0] << ", "
            << net->params()[1].weight_ih.sizes()[1] << "] -- [hidden_size, hidden_size] (fed by layer 0's output)\n";

  return 0;
}
```

Output

```text
params().size()=2
layer 0 weight_ih shape: [5, 3] -- [hidden_size, input_size]
layer 1 weight_ih shape: [5, 5] -- [hidden_size, hidden_size] (fed by layer 0's output)
```

### Example 2

Setting a block's `weight_ih`/`weight_hh`/`bias` by hand, in place, via
`copy_` -- the exact pattern every hand-verified `forward` example
depends on.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(1, 1);
  net->params()[0].weight_ih.copy_(pluss::make_series({2.0}, {1, 1}));
  net->params()[0].weight_hh.copy_(pluss::make_series({0.0}, {1, 1}));
  net->params()[0].bias.copy_(pluss::make_series({0.5}, {1}));

  std::cout << "weight_ih: " << net->params()[0].weight_ih << "\n";
  std::cout << "weight_hh: " << net->params()[0].weight_hh << "\n";
  std::cout << "bias: " << net->params()[0].bias << "\n";

  return 0;
}
```

Output

```text
weight_ih: pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[2]]
weight_hh: pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[0]]
bias: pluss::Series(shape=[1], strides=[1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[0.5]
```

### Example 3

The `const` overload (reached from a `const RNN&`) sees exactly the same
values as the mutable one -- it only removes the ability to mutate them.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(1, 1);
  net->params()[0].weight_ih.copy_(pluss::make_series({2.0}, {1, 1}));

  const nn::RNN& const_net = net;
  std::cout << "const overload weight_ih: " << const_net->params()[0].weight_ih << "\n";

  return 0;
}
```

Output

```text
const overload weight_ih: pluss::Series(shape=[1,1], strides=[1,1], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=[[2]]
```

## See Also

- [`forward`](forward.md) -- reads these weight blocks every call.
- [`options`](options.md) -- the geometry/behavior fields these blocks'
  shapes are derived from.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html">
    <span class="label">Previous</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
