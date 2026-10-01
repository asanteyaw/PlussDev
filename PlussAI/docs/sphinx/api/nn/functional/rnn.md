# `pluss::ai::nn::rnn`

Stateless Elman-RNN forward over explicit per-layer weights, given a
`NetConfig`; what RNN's forward calls internally. Also the
namespace-like scope for the `rnn::NetWeight` / `rnn::NetConfig` types.

## Signature

```cpp
std::tuple<Series, Series> nn::rnn(
    const Series& input, const Series& hx,
    pluss::RangeView<nn::rnn::NetWeight> params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, Series> nn::rnn(
    const Series& input, pluss::RangeView<nn::rnn::NetWeight> params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, Series> nn::rnn(
    const Series& input, const Series& hx,
    const nn::rnn::NetWeight& params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, Series> nn::rnn(
    const Series& input, const nn::rnn::NetWeight& params, nn::rnn::NetConfig cfg = {});
```

## What It Does

Runs the exact same multi-layer, multi-timestep Elman-RNN computation
[`RNN::forward`](../rnn/RNN/forward.md) does -- `h_t =
nonlinearity(x_t @ weight_ih^T + bias + h_{t-1} @ weight_hh^T)` per
timestep, per layer -- but over explicitly-supplied
[`NetWeight`](NetWeight.md) blocks and a [`NetConfig`](NetConfig.md)
instead of a constructed `RNN` instance's own registered parameters.
`rnn` is simultaneously a **callable function** and, through a
deliberate name-hiding technique (an injected class name coexisting with
a same-named function, resolved differently by qualified versus
unqualified lookup), a **scope holding two nested type aliases**:
`nn::rnn::NetWeight` and `nn::rnn::NetConfig`. Both uses work
simultaneously in the same program -- see Example 1. Four overloads:
`hx` may be supplied explicitly or omitted (defaulting to zeros), and
`params` may be a single [`NetWeight`](NetWeight.md) (one layer, one
direction) or a `RangeView` of them (multiple layers and/or
bidirectional -- exactly `num_layers * (bidirectional ? 2 : 1)` blocks,
required in layer-major, direction-minor order).

## Parameters

`const Series& input`
: `[seq_len, batch, input_size]` (or `[batch, seq_len, input_size]` when
  `cfg.batch_first == true`).

`const Series& hx`
: `[num_layers * dirs, batch, hidden_size]`, the initial hidden state.
  Omit entirely (the two-argument overloads) for zeros.

`params`
: One [`NetWeight`](NetWeight.md) block per `(layer, direction)` pair --
  a single block for a 1-layer, unidirectional call, or a `RangeView` of
  exactly `cfg.num_layers * (cfg.bidirectional ? 2 : 1)` blocks
  otherwise, in layer-major, direction-minor order (matching
  [`RNN::params()`](../rnn/RNN/params.md)'s own ordering exactly).

`nn::rnn::NetConfig cfg`
: See [`NetConfig`](NetConfig.md). Governs the layout convention,
  nonlinearity, bias layout, and every other geometry/behavior choice
  this call needs, since there is no constructed `RNN` instance to read
  them from.

## Returns

`{output, hidden}`: `output` is `[seq_len, batch, hidden_size * dirs]`
(or batch-first, per `cfg.batch_first`); `hidden` is `[num_layers * dirs,
batch, hidden_size]`, the final hidden state of every layer/direction.

## Errors

- `params`'s block count does not match `cfg.num_layers * (cfg.bidirectional
  ? 2 : 1)`, or a block's bias layout does not match `cfg.use_bias`/
  `cfg.split_bias` -- throws.

## Practical Examples

Four examples: the tag-hiding trick itself (calling `rnn(...)` and
naming `rnn::NetWeight` in the same program) with a hand-verified
result, cross-checking a live `RNN` instance's own `params()` fed
straight into this function, the two-argument (no explicit `hx`)
overload matching an explicit zero `hx`, and a bidirectional call
requiring two weight blocks.

### Example 1

`nn::rnn::NetWeight`/`nn::rnn::NetConfig` name the nested types; `nn::rnn(...)`
calls the function -- both resolve correctly in the same program. The
single-timestep, single-feature result is hand-verifiable against the
plain Elman-RNN formula.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <cmath>

int main() {
  namespace nn = pluss::ai::nn;

  // confirms the tag-hiding trick: nn::rnn is both a callable function
  // AND a scope for nested types (NetWeight, NetConfig), simultaneously
  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.1, 0.2}, {1, 2});  // hidden_size=1, input_size=2
  params.weight_hh = pluss::make_series({0.3}, {1, 1});
  params.bias = pluss::make_series({0.05}, {1});

  nn::rnn::NetConfig cfg{};
  cfg.nonlinearity = "tanh";

  // input: [seq_len=1, batch=1, input_size=2]
  const pluss::Series input = pluss::make_series({1.0, 1.0}, {1, 1, 2});
  const pluss::Series hx = pluss::make_series({0.0}, {1, 1, 1});

  const auto [output, hidden] = nn::rnn(input, hx, params, cfg);

  std::cout << "output: " << output << "\n";
  std::cout << "hand check: tanh(1*0.1 + 1*0.2 + 0*0.3 + 0.05) = tanh(0.35) = "
            << std::tanh(0.35) << "\n";

  return 0;
}
```

Output

```text
output: pluss::Series(shape=[1,1,1], strides=[1,1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[[0.33637554]]]
hand check: tanh(1*0.1 + 1*0.2 + 0*0.3 + 0.05) = tanh(0.35) = 0.336376
```

### Example 2

A live [`RNN`](../rnn/RNN.md) instance's own `params()` and `options()`
fed directly into this function, alongside its `forward` call on the
same input -- bit-for-bit identical.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(3, 4, /*num_layers=*/2);
  const pluss::Series input = pluss::make_series(
      {1.0, 0.0, 0.0,
       0.0, 1.0, 0.0},
      {2, 1, 3});  // seq_len=2, batch=1, input_size=3

  const auto [module_output, module_hidden] = net.forward(input);

  nn::rnn::NetConfig cfg{};
  cfg.num_layers = net->options().num_layers;
  cfg.nonlinearity = net->options().nonlinearity;
  cfg.batch_first = net->options().batch_first;

  const auto [func_output, func_hidden] = nn::rnn(input, net->params(), cfg);

  std::cout << "RNN::forward output:  " << module_output.reshape({2, 4}) << "\n";
  std::cout << "nn::rnn(...) output:  " << func_output.reshape({2, 4}) << "\n";
  std::cout << "identical: " << std::boolalpha << module_output.allclose(func_output) << "\n";

  return 0;
}
```

Output

```text
RNN::forward output:  pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[0.06711339, -0.62786124, -0.21437992, -0.46105605],
  [-0.17479272, -0.60670408, -0.40527927, -0.084670413]]
nn::rnn(...) output:  pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[0.06711339, -0.62786124, -0.21437992, -0.46105605],
  [-0.17479272, -0.60670408, -0.40527927, -0.084670413]]
identical: true
```

### Example 3

Omitting `hx` entirely (the two-argument-before-`params` overload)
produces exactly the same result as passing an explicit all-zeros `hx`
of the correct shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.5}, {1, 1});
  params.weight_hh = pluss::make_series({0.0}, {1, 1});  // hidden feedback zeroed for clarity

  const pluss::Series input = pluss::make_series({2.0}, {1, 1, 1});

  nn::rnn::NetConfig cfg{};
  cfg.use_bias = false;

  // overload with no hx -- defaults to zeros internally
  const auto [out_no_hx, hidden_no_hx] = nn::rnn(input, params, cfg);
  // explicit zero hx -- must match exactly
  const pluss::Series zero_hx = pluss::series::zeros({1, 1, 1}, pluss::Config{});
  const auto [out_explicit, hidden_explicit] = nn::rnn(input, zero_hx, params, cfg);

  std::cout << "no hx (defaults to zeros):     " << out_no_hx.item<double>() << "\n";
  std::cout << "explicit zero hx:              " << out_explicit.item<double>() << "\n";
  std::cout << "identical: " << std::boolalpha << out_no_hx.allclose(out_explicit) << "\n";

  return 0;
}
```

Output

```text
no hx (defaults to zeros):     0.761594
explicit zero hx:              0.761594
identical: true
```

### Example 4

`bidirectional=true` requires exactly two [`NetWeight`](NetWeight.md)
blocks (forward direction, then backward) -- confirmed here by feeding a
live bidirectional `RNN`'s own two-block `params()` through a
`RangeView` into this function.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(2, 3, /*num_layers=*/1, "tanh", /*bias=*/true, /*batch_first=*/false,
              /*dropout=*/0.0, /*bidirectional=*/true);
  const pluss::Series input = pluss::make_series(
      {1.0, 0.0,
       0.0, 1.0,
       1.0, 1.0},
      {3, 1, 2});  // seq_len=3, batch=1, input_size=2

  const auto [module_output, module_hidden] = net.forward(input);

  nn::rnn::NetConfig cfg{};
  cfg.bidirectional = true;
  cfg.nonlinearity = "tanh";

  // bidirectional needs 2 weight blocks (forward + backward direction)
  std::vector<nn::rnn::NetWeight> params_vec(net->params().begin(), net->params().end());
  const auto [func_output, func_hidden] = nn::rnn(
      input, pluss::RangeView<nn::rnn::NetWeight>(params_vec), cfg);

  std::cout << "params blocks: " << params_vec.size() << " (forward + backward direction)\n";
  std::cout << "identical: " << std::boolalpha << module_output.allclose(func_output) << "\n";

  return 0;
}
```

Output

```text
params blocks: 2 (forward + backward direction)
identical: true
```

## See Also

- [`NetWeight`](NetWeight.md) / [`NetConfig`](NetConfig.md) -- the two
  types this function's own name also serves as a scope for.
- [`gru`](gru.md) / [`lstm`](lstm.md) -- the gated counterparts, same
  calling convention.
- [`RNN`](../rnn/RNN.md) -- the stateful `Module` wrapper this function
  powers.
