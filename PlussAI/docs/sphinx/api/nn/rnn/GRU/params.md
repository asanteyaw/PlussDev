<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d7b083bb5a52 -->

# `pluss::ai::nn::GRU::params`

[Back to `pluss::ai::nn::GRU`](../GRU.md)

Only reachable via `->` (`net->params()`).

## Signature

```cpp
std::vector<pluss::ai::rnn::NetWeight>& nn::GRU::params();
const std::vector<pluss::ai::rnn::NetWeight>& nn::GRU::params() const;
```

## What It Does

Exposes the raw per-`(layer, direction)` weight blocks
[`forward`](forward.md) reads from directly -- the same registered
`Series` objects `parameters()`/`named_parameters()` (inherited from
`Module`) also see, just grouped by block instead of flattened. Reach for
`params()` when a specific block's weights need to be inspected or set by
hand; reach for `named_parameters()` when what matters is the dotted name
each tensor was registered under.

## Parameters

None.

## Returns

A `std::vector<pluss::ai::rnn::NetWeight>&` (or `const`), one entry per
`(layer, direction)` pair, ordered layer-major then direction-minor. Each
`NetWeight` has:

- `weight_ih` -- `[3 * hidden_size, layer_input_size]`, three gates'
  worth of input-to-hidden projection stacked in `(reset, update,
  candidate)` order.
- `weight_hh` -- `[3 * hidden_size, hidden_size]`, the matching
  hidden-to-hidden projection.
- `bias` -- `[3 * hidden_size]`, defined when `options().bias == true`
  and `options().split_bias == false`; undefined otherwise.
- `bias_ih` / `bias_hh` -- `[3 * hidden_size]` each, defined when
  `options().split_bias == true`; undefined otherwise.
- `weight_hr` -- always undefined for `GRU`.

## Errors

None.

## Practical Examples

Three examples: reading a fresh instance's block count and shapes, setting
`weight_ih` directly and confirming the gate order is preserved as
written, and the `const` overload.

### Example 1

A 2-layer, unidirectional `GRU` has 2 weight blocks, and every
`weight_ih` has `3 * hidden_size` rows -- the gate stacking, not
`hidden_size` alone.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRUOptions opts{};
  opts.input_size = 3;
  opts.hidden_size = 5;
  opts.num_layers = 2;
  nn::GRU net(opts);

  std::cout << "params().size()=" << net->params().size() << "\n";
  std::cout << "layer 0 weight_ih shape: [" << net->params()[0].weight_ih.sizes()[0] << ", "
            << net->params()[0].weight_ih.sizes()[1] << "] -- [3*hidden_size, input_size]\n";
  std::cout << "layer 1 weight_ih shape: [" << net->params()[1].weight_ih.sizes()[0] << ", "
            << net->params()[1].weight_ih.sizes()[1] << "] -- [3*hidden_size, hidden_size] (fed by layer 0's output)\n";

  return 0;
}
```

Output

```text
params().size()=2
layer 0 weight_ih shape: [15, 3] -- [3*hidden_size, input_size]
layer 1 weight_ih shape: [15, 5] -- [3*hidden_size, hidden_size] (fed by layer 0's output)
```

### Example 2

Setting `weight_ih` directly, in `(reset, update, candidate)` row order.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(1, 1);
  net->params()[0].weight_ih.copy_(pluss::make_series({1.0, 2.0, 3.0}, {3, 1}));

  std::cout << "weight_ih (gate order r,z,n): " << net->params()[0].weight_ih << "\n";

  return 0;
}
```

Output

```text
weight_ih (gate order r,z,n): pluss::Series(shape=[3,1], strides=[1,1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[[1],
  [2],
  [3]]
```

### Example 3

The `const` overload sees exactly the same values as the mutable one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(1, 1);
  net->params()[0].weight_ih.copy_(pluss::make_series({1.0, 2.0, 3.0}, {3, 1}));

  const nn::GRU& const_net = net;
  std::cout << "const overload weight_ih: " << const_net->params()[0].weight_ih << "\n";

  return 0;
}
```

Output

```text
const overload weight_ih: pluss::Series(shape=[3,1], strides=[1,1], dtype=float32, platform=cpu, numel=3, contiguous=true)
data=[[1],
  [2],
  [3]]
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
