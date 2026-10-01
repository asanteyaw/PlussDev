# `pluss::ai::nn::lstm`

Stateless LSTM forward over explicit per-layer weights; what LSTM's
forward calls internally.

## Signature

```cpp
using nn::rnn::LSTMState = std::tuple<Series, Series>;  // {hidden, cell}

std::tuple<Series, nn::rnn::LSTMState> nn::lstm(
    const Series& input, const nn::rnn::LSTMState& state,
    pluss::RangeView<nn::rnn::NetWeight> params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, nn::rnn::LSTMState> nn::lstm(
    const Series& input, const Series& hx, const Series& cx,
    pluss::RangeView<nn::rnn::NetWeight> params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, nn::rnn::LSTMState> nn::lstm(
    const Series& input, pluss::RangeView<nn::rnn::NetWeight> params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, nn::rnn::LSTMState> nn::lstm(
    const Series& input, const nn::rnn::LSTMState& state,
    const nn::rnn::NetWeight& params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, nn::rnn::LSTMState> nn::lstm(
    const Series& input, const Series& hx, const Series& cx,
    const nn::rnn::NetWeight& params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, nn::rnn::LSTMState> nn::lstm(
    const Series& input, const nn::rnn::NetWeight& params, nn::rnn::NetConfig cfg = {});
```

## What It Does

Runs the exact same multi-layer, multi-timestep doubly-gated computation
[`LSTM::forward`](../rnn/LSTM/forward.md) does -- gate order
`(input, forget, cell, output)`, `i, f, o = sigmoid(...)`, `g =
tanh(...)`, `c_t = f * c_{t-1} + i * g`, `h_t = o * tanh(c_t)`, and
(when a block's `weight_hr` is defined) `h_t = h_t @ weight_hr^T` for
the recurrent projection -- over explicitly-supplied
[`NetWeight`](NetWeight.md) blocks and a [`NetConfig`](NetConfig.md).
Six overloads in total, more than [`rnn`](rnn.md)/[`gru`](gru.md)'s
four, since LSTM carries **two** recurrent states (`hidden`, `cell`)
that can be passed either bundled as one
`LSTMState` (`std::tuple<Series, Series>`) or as two separate
`hx`/`cx` arguments -- both forms are fully equivalent, see Example 3.
Like the other two, `params` may be a single block or a `RangeView` of
`num_layers * (bidirectional ? 2 : 1)` of them.

## Parameters

`const Series& input`
: `[seq_len, batch, input_size]` (or batch-first).

`const nn::rnn::LSTMState& state` / `const Series& hx, const Series& cx`
: The initial `{hidden, cell}` state, bundled or separate. Omit entirely
  for zeros on both.

`params`
: One [`NetWeight`](NetWeight.md) per `(layer, direction)` pair;
  `weight_ih`/`weight_hh` each `[4 * hidden_size, ...]` in `(input,
  forget, cell, output)` row order. `weight_hr`, when defined, projects
  `hidden_size` down to `cfg.proj_size`.

`nn::rnn::NetConfig cfg`
: See [`NetConfig`](NetConfig.md). `cfg.proj_size > 0` activates the
  recurrent projection, reading each block's own `weight_hr`.

## Returns

`{output, {hidden, cell}}`: `output`'s and `hidden`'s last axis is
`proj_size` when projecting, `hidden_size` otherwise; `cell`'s last axis
is always `hidden_size`, regardless of projection.

## Errors

- `params`'s block count or bias layout mismatched against `cfg` --
  throws.

## Practical Examples

Five examples: a hand-verified single-timestep gate computation
(including the final cell state), a live `LSTM` instance's `params()`
fed through this function matching `forward` exactly, the bundled
`LSTMState` and separate `hx`/`cx` overloads agreeing exactly (plus the
no-state overload defaulting both to zeros), `proj_size` producing a
narrower hidden state than cell state, and a bidirectional call requiring
two weight blocks.

### Example 1

With hidden-to-hidden feedback weights zeroed out, both the output and
the final cell state are hand-verifiable directly from the gate formulas.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <cmath>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.1, 0.2, 0.3, 0.4}, {4, 1});  // gate order (input, forget, cell, output)
  params.weight_hh = pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4, 1});  // zeroed hidden feedback for clarity
  params.bias = pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4});

  nn::rnn::NetConfig cfg{};

  const pluss::Series input = pluss::make_series({1.0}, {1, 1, 1});
  const pluss::Series hx = pluss::make_series({0.0}, {1, 1, 1});
  const pluss::Series cx = pluss::make_series({0.2}, {1, 1, 1});

  const auto [output, state] = nn::lstm(input, nn::rnn::LSTMState{hx, cx}, params, cfg);
  const auto& [final_hidden, final_cell] = state;

  const double i = 1.0 / (1.0 + std::exp(-0.1));
  const double f = 1.0 / (1.0 + std::exp(-0.2));
  const double g = std::tanh(0.3);
  const double o = 1.0 / (1.0 + std::exp(-0.4));
  const double c_t = f * 0.2 + i * g;
  const double h_t = o * std::tanh(c_t);

  std::cout << "output:      " << output.item<double>() << "\n";
  std::cout << "hand check (h_t = o*tanh(f*c_prev + i*g)): " << h_t << "\n";
  std::cout << "final cell:  " << final_cell.item<double>() << ", hand check c_t=" << c_t << "\n";

  return 0;
}
```

Output

```text
output:      0.153866
hand check (h_t = o*tanh(f*c_prev + i*g)): 0.153866
final cell:  0.2629, hand check c_t=0.2629
```

### Example 2

A live [`LSTM`](../rnn/LSTM.md) instance's `params()` fed directly into
this function matches `forward`'s own output exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(3, 4, /*num_layers=*/2);
  const pluss::Series input = pluss::make_series(
      {1.0, 0.0, 0.0,
       0.0, 1.0, 0.0},
      {2, 1, 3});

  const auto [module_output, module_state] = net.forward(input);

  nn::rnn::NetConfig cfg{};
  cfg.num_layers = net->options().num_layers;
  cfg.batch_first = net->options().batch_first;

  const auto [func_output, func_state] = nn::lstm(input, net->params(), cfg);

  std::cout << "identical: " << std::boolalpha << module_output.allclose(func_output) << "\n";

  return 0;
}
```

Output

```text
identical: true
```

### Example 3

The bundled `LSTMState` form and the separate `hx`/`cx` form compute the
exact same result; the no-state overload defaults both to zeros.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.1, 0.2, 0.3, 0.4}, {4, 1});
  params.weight_hh = pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4, 1});
  params.bias = pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4});

  const pluss::Series input = pluss::make_series({1.0}, {1, 1, 1});
  const pluss::Series hx = pluss::make_series({0.0}, {1, 1, 1});
  const pluss::Series cx = pluss::make_series({0.2}, {1, 1, 1});

  // tuple-state overload
  const auto [out_tuple, state_tuple] = nn::lstm(input, nn::rnn::LSTMState{hx, cx}, params);
  // separate hx/cx overload
  const auto [out_separate, state_separate] = nn::lstm(input, hx, cx, params);

  std::cout << "LSTMState-tuple form and separate hx/cx form agree: "
            << std::boolalpha << out_tuple.allclose(out_separate) << "\n";

  // no-state overload -- defaults both hidden and cell to zeros
  const auto [out_no_state, state_no_state] = nn::lstm(input, params);
  std::cout << "no-state overload output: " << out_no_state.item<double>() << "\n";

  return 0;
}
```

Output

```text
LSTMState-tuple form and separate hx/cx form agree: true
no-state overload output: 0.0908519
```

### Example 4

`cfg.proj_size=3` (matching a live `LSTM` instance built the same way)
produces a `hidden` state narrower than `hidden_size` -- `[1, 1, 3]`
here, against an `8`-wide `hidden_size` -- while `cell` stays
`hidden_size`-wide.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(4, 8, /*num_layers=*/1, /*bias=*/true, /*batch_first=*/false,
              /*dropout=*/0.0, /*bidirectional=*/false, /*split_bias=*/false, /*proj_size=*/3);
  const pluss::Series input = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4});

  const auto [module_output, module_state] = net.forward(input);
  const auto& [module_hidden, module_cell] = module_state;

  nn::rnn::NetConfig cfg{};
  cfg.proj_size = 3;

  const auto [func_output, func_state] = nn::lstm(input, net->params()[0], cfg);
  const auto& [func_hidden, func_cell] = func_state;

  std::cout << "module hidden shape: [" << module_hidden.sizes()[0] << "," << module_hidden.sizes()[1] << "," << module_hidden.sizes()[2] << "]\n";
  std::cout << "func hidden shape:   [" << func_hidden.sizes()[0] << "," << func_hidden.sizes()[1] << "," << func_hidden.sizes()[2] << "]\n";
  std::cout << "identical output: " << std::boolalpha << module_output.allclose(func_output) << "\n";

  return 0;
}
```

Output

```text
module hidden shape: [1,1,3]
func hidden shape:   [1,1,3]
identical output: true
```

### Example 5

`bidirectional=true` requires exactly two [`NetWeight`](NetWeight.md)
blocks.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(2, 3, /*num_layers=*/1, /*bias=*/true, /*batch_first=*/false,
              /*dropout=*/0.0, /*bidirectional=*/true);
  const pluss::Series input = pluss::make_series(
      {1.0, 0.0,
       0.0, 1.0,
       1.0, 1.0},
      {3, 1, 2});

  const auto [module_output, module_state] = net.forward(input);

  nn::rnn::NetConfig cfg{};
  cfg.bidirectional = true;

  std::vector<nn::rnn::NetWeight> params_vec(net->params().begin(), net->params().end());
  const auto [func_output, func_state] = nn::lstm(
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

- [`NetWeight`](NetWeight.md) / [`NetConfig`](NetConfig.md) -- the
  explicit weight/configuration types this function takes.
- [`rnn`](rnn.md) / [`gru`](gru.md) -- the plain and singly-gated
  counterparts, same calling convention minus the second (`cell`) state.
- [`LSTM`](../rnn/LSTM.md) -- the stateful `Module` wrapper this
  function powers.
