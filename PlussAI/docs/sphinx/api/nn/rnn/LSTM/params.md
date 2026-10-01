<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cf6b9c013eea -->

# `pluss::ai::nn::LSTM::params`

[Back to `pluss::ai::nn::LSTM`](../LSTM.md)

Only reachable via `->` (`net->params()`).

## Signature

```cpp
std::vector<pluss::ai::rnn::NetWeight>& nn::LSTM::params();
const std::vector<pluss::ai::rnn::NetWeight>& nn::LSTM::params() const;
```

## What It Does

Exposes the raw per-`(layer, direction)` weight blocks
[`forward`](forward.md) reads from directly. Reach for `params()` when a
specific block's weights need to be inspected or set by hand; reach for
`named_parameters()` (inherited from `Module`) when what matters is the
dotted name each tensor was registered under.

## Parameters

None.

## Returns

A `std::vector<pluss::ai::rnn::NetWeight>&` (or `const`), one entry per
`(layer, direction)` pair. Each `NetWeight` has:

- `weight_ih` -- `[4 * hidden_size, layer_input_size]`, four gates'
  worth of input-to-hidden projection, in `(input, forget, cell,
  output)` order.
- `weight_hh` -- `[4 * hidden_size, recurrent_size]`, where
  `recurrent_size` is `hidden_size` when `options().proj_size == 0`, or
  `proj_size` when it is not (the recurrent connection runs on the
  already-projected hidden state -- see
  [`Introduction`](../Introduction.md)).
- `bias` -- `[4 * hidden_size]`, defined when `options().bias == true`
  and `options().split_bias == false`; undefined otherwise.
- `bias_ih` / `bias_hh` -- `[4 * hidden_size]` each, defined when
  `options().split_bias == true`; undefined otherwise.
- `weight_hr` -- `[proj_size, hidden_size]`, defined only when
  `options().proj_size > 0`; undefined otherwise. The only `NetWeight`
  field any of `RNN`/`GRU` ever leaves permanently undefined -- `LSTM` is
  the sole consumer of this field.

## Errors

None.

## Practical Examples

Three examples: reading a fresh instance's block count and shapes
(including confirming `weight_hr` is undefined without a projection),
setting `weight_ih` directly, and the `const` overload.

### Example 1

A 2-layer, unidirectional `LSTM` has 2 weight blocks, every `weight_ih`
has `4 * hidden_size` rows, and `weight_hr` stays undefined when
`proj_size` is left at its default `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTMOptions opts{};
  opts.input_size = 3;
  opts.hidden_size = 5;
  opts.num_layers = 2;
  nn::LSTM net(opts);

  std::cout << "params().size()=" << net->params().size() << "\n";
  std::cout << "layer 0 weight_ih shape: [" << net->params()[0].weight_ih.sizes()[0] << ", "
            << net->params()[0].weight_ih.sizes()[1] << "] -- [4*hidden_size, input_size]\n";
  std::cout << "layer 1 weight_ih shape: [" << net->params()[1].weight_ih.sizes()[0] << ", "
            << net->params()[1].weight_ih.sizes()[1] << "] -- [4*hidden_size, hidden_size] (fed by layer 0's output)\n";
  std::cout << "weight_hr defined (proj_size=0): " << std::boolalpha << net->params()[0].weight_hr.defined() << "\n";

  return 0;
}
```

Output

```text
params().size()=2
layer 0 weight_ih shape: [20, 3] -- [4*hidden_size, input_size]
layer 1 weight_ih shape: [20, 5] -- [4*hidden_size, hidden_size] (fed by layer 0's output)
weight_hr defined (proj_size=0): false
```

### Example 2

Setting `weight_ih` directly, in `(input, forget, cell, output)` row
order.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(1, 1);
  net->params()[0].weight_ih.copy_(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1}));

  std::cout << "weight_ih (gate order i,f,g,o): " << net->params()[0].weight_ih << "\n";

  return 0;
}
```

Output

```text
weight_ih (gate order i,f,g,o): pluss::Series(shape=[4,1], strides=[1,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [2],
  [3],
  [4]]
```

### Example 3

The `const` overload sees exactly the same values as the mutable one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(1, 1);
  net->params()[0].weight_ih.copy_(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4, 1}));

  const nn::LSTM& const_net = net;
  std::cout << "const overload weight_ih: " << const_net->params()[0].weight_ih << "\n";

  return 0;
}
```

Output

```text
const overload weight_ih: pluss::Series(shape=[4,1], strides=[1,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [2],
  [3],
  [4]]
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
