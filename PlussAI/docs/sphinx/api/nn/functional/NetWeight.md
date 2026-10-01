# `pluss::ai::nn::rnn::NetWeight`

One recurrent layer's explicit weights: input-hidden, hidden-hidden,
biases, and an optional recurrent projection.

## Signature

```cpp
Series weight_ih;
Series weight_hh;
Series bias;
Series bias_ih;
Series bias_hh;
Series weight_hr;

bool has_bias() const noexcept;
bool has_projection() const noexcept;
```

A plain aggregate -- there is no constructor beyond the implicit
default/aggregate ones. This is exactly what
[`RNN::params()`](../rnn/RNN/params.md) /
[`GRU::params()`](../rnn/GRU/params.md) /
[`LSTM::params()`](../rnn/LSTM/params.md) hold, one entry per `(layer,
direction)` pair -- reachable to inspect directly, or to hand-build for
[`rnn`](rnn.md)/[`gru`](gru.md)/[`lstm`](lstm.md) without constructing
any `Module` at all (see Example 3).

## State

`weight_ih` -- `Series`
: `[gates * hidden_size, layer_input_size]`, where `gates` is `1` for
  [`rnn`](rnn.md), `3` for [`gru`](gru.md), `4` for [`lstm`](lstm.md).

`weight_hh` -- `Series`
: `[gates * hidden_size, recurrent_size]`, where `recurrent_size` is
  `hidden_size` ordinarily, or `proj_size` when `weight_hr` is defined
  (the recurrent connection runs on the already-projected state).

`bias` -- `Series`
: `[gates * hidden_size]`, defined when a unified (non-split) bias is
  used.

`bias_ih` / `bias_hh` -- `Series`
: `[gates * hidden_size]` each, defined when a split bias is used
  instead of `bias`.

`weight_hr` -- `Series`
: `[proj_size, hidden_size]`, defined only for a projected `lstm` call
  (undefined otherwise, and undefined unconditionally for
  [`rnn`](rnn.md)/[`gru`](gru.md), which never consult it).

## Methods

`has_bias()` -- `bool`
: `true` when any of `bias`/`bias_ih`/`bias_hh` is defined. `false` for
  a default-constructed (all-undefined) instance.

`has_projection()` -- `bool`
: `true` when `weight_hr` is defined.

## Practical Examples

Three examples: `has_bias()`/`has_projection()` across a few field
combinations, inspecting a live `GRU` instance's own `params()[0]`
directly, and feeding a hand-built `NetWeight` (no `Module` constructed
at all) into [`rnn`](rnn.md).

### Example 1

An empty instance reports `false` for both; defining any bias field
(unified or split) makes `has_bias()` `true`; defining `weight_hr` makes
`has_projection()` `true`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetWeight empty{};
  std::cout << "empty NetWeight: has_bias()=" << std::boolalpha << empty.has_bias()
            << ", has_projection()=" << empty.has_projection() << "\n";

  nn::rnn::NetWeight with_bias{};
  with_bias.weight_ih = pluss::make_series({0.1}, {1, 1});
  with_bias.weight_hh = pluss::make_series({0.2}, {1, 1});
  with_bias.bias = pluss::make_series({0.05}, {1});
  std::cout << "with unified bias: has_bias()=" << with_bias.has_bias() << "\n";

  nn::rnn::NetWeight with_split_bias{};
  with_split_bias.bias_ih = pluss::make_series({0.1}, {1});
  with_split_bias.bias_hh = pluss::make_series({0.2}, {1});
  std::cout << "with split bias (bias_ih/bias_hh only): has_bias()=" << with_split_bias.has_bias() << "\n";

  nn::rnn::NetWeight with_projection{};
  with_projection.weight_hr = pluss::make_series({0.3}, {1, 1});
  std::cout << "with weight_hr defined: has_projection()=" << with_projection.has_projection() << "\n";

  return 0;
}
```

Output

```text
empty NetWeight: has_bias()=false, has_projection()=false
with unified bias: has_bias()=true
with split bias (bias_ih/bias_hh only): has_bias()=true
with weight_hr defined: has_projection()=true
```

### Example 2

A live [`GRU`](../rnn/GRU.md) instance's own `params()[0]` -- `weight_ih`/
`weight_hh` are `3 * hidden_size`-wide (the gate stacking), and
`has_projection()` is always `false` for `GRU`, which never uses
`weight_hr`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(3, 5);
  const nn::rnn::NetWeight& block = net->params()[0];

  std::cout << "weight_ih shape: [" << block.weight_ih.sizes()[0] << ", " << block.weight_ih.sizes()[1]
            << "] -- [3*hidden_size, input_size]\n";
  std::cout << "weight_hh shape: [" << block.weight_hh.sizes()[0] << ", " << block.weight_hh.sizes()[1]
            << "] -- [3*hidden_size, hidden_size]\n";
  std::cout << "has_bias()=" << std::boolalpha << block.has_bias() << "\n";
  std::cout << "has_projection()=" << block.has_projection() << " -- GRU never uses weight_hr\n";

  return 0;
}
```

Output

```text
weight_ih shape: [15, 3] -- [3*hidden_size, input_size]
weight_hh shape: [15, 5] -- [3*hidden_size, hidden_size]
has_bias()=true
has_projection()=false -- GRU never uses weight_hr
```

### Example 3

A `NetWeight` built entirely by hand, with no `Module` constructed at
all, fed directly into [`rnn`](rnn.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // hand-built NetWeight, no Module involved at all
  nn::rnn::NetWeight block{};
  block.weight_ih = pluss::make_series({0.5}, {1, 1});
  block.weight_hh = pluss::make_series({0.0}, {1, 1});
  block.bias = pluss::make_series({0.0}, {1});

  const pluss::Series input = pluss::make_series({2.0}, {1, 1, 1});
  const auto [output, hidden] = nn::rnn(input, block);

  std::cout << "output from a hand-built NetWeight, no Module constructed: " << output.item<double>() << "\n";

  return 0;
}
```

Output

```text
output from a hand-built NetWeight, no Module constructed: 0.761594
```

## Changelog

- **0.1** — the explicit per-layer weight/bias/projection bundle
  `rnn`/`gru`/`lstm` take directly; `has_bias()`/`has_projection()`.

## See Also

- [`NetConfig`](NetConfig.md) -- the companion configuration struct.
- [`rnn`](rnn.md) / [`gru`](gru.md) / [`lstm`](lstm.md) -- the three
  consumers of this type.
- [`RNN::params()`](../rnn/RNN/params.md) /
  [`GRU::params()`](../rnn/GRU/params.md) /
  [`LSTM::params()`](../rnn/LSTM/params.md) -- the stateful `Module`
  accessors returning exactly this type.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
