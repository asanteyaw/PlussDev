# `pluss::ai::nn::gru`

Stateless GRU forward over explicit per-layer weights; what GRU's
forward calls internally.

## Signature

```cpp
std::tuple<Series, Series> nn::gru(
    const Series& input, const Series& hx,
    pluss::RangeView<nn::rnn::NetWeight> params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, Series> nn::gru(
    const Series& input, pluss::RangeView<nn::rnn::NetWeight> params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, Series> nn::gru(
    const Series& input, const Series& hx,
    const nn::rnn::NetWeight& params, nn::rnn::NetConfig cfg = {});
std::tuple<Series, Series> nn::gru(
    const Series& input, const nn::rnn::NetWeight& params, nn::rnn::NetConfig cfg = {});
```

## What It Does

Runs the exact same multi-layer, multi-timestep gated computation
[`GRU::forward`](../rnn/GRU/forward.md) does -- gate order
`(reset, update, candidate)`, `r = sigmoid(gi_r + gh_r)`, `z =
sigmoid(gi_z + gh_z)`, `n = tanh(gi_n + r * gh_n)`, `h_t = n + z *
(h_{t-1} - n)` per timestep, per layer -- over explicitly-supplied
[`NetWeight`](NetWeight.md) blocks and a [`NetConfig`](NetConfig.md),
field-for-field the same call shape as [`rnn`](rnn.md): four overloads
(`hx` explicit or omitted, `params` a single block or a `RangeView` of
`num_layers * (bidirectional ? 2 : 1)` of them). `NetConfig::nonlinearity`
is not consulted at all here -- the gate nonlinearities (`sigmoid` for
`r`/`z`, `tanh` for `n`) are fixed by the GRU architecture itself, unlike
[`rnn`](rnn.md)'s single configurable nonlinearity.

## Parameters

`const Series& input`
: `[seq_len, batch, input_size]` (or batch-first).

`const Series& hx`
: `[num_layers * dirs, batch, hidden_size]`. Omit for zeros.

`params`
: One [`NetWeight`](NetWeight.md) per `(layer, direction)` pair;
  `weight_ih`/`weight_hh` each `[3 * hidden_size, ...]`, three gates'
  worth stacked in `(reset, update, candidate)` row order.

`nn::rnn::NetConfig cfg`
: See [`NetConfig`](NetConfig.md).

## Returns

`{output, hidden}`, identical shapes to [`rnn`](rnn.md)'s own return.

## Errors

- `params`'s block count or bias layout mismatched against `cfg` --
  throws.

## Practical Examples

Four examples: a hand-verified single-timestep gate computation, a live
`GRU` instance's `params()` fed through this function matching
`forward` exactly, `dropout` reproducibility across layers with a fixed
seed (and `training=false` skipping it entirely), and a bidirectional
call requiring two weight blocks.

### Example 1

With hidden-to-hidden feedback weights zeroed out for clarity, the
result reduces to `n + z * (h_prev - n)` computed purely from the
input-side gates -- hand-verified independently.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <cmath>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.1, 0.2, 0.3}, {3, 1});  // gate order (reset, update, candidate)
  params.weight_hh = pluss::make_series({0.0, 0.0, 0.0}, {3, 1});  // zeroed hidden feedback for clarity
  params.bias = pluss::make_series({0.0, 0.0, 0.0}, {3});

  nn::rnn::NetConfig cfg{};

  const pluss::Series input = pluss::make_series({1.0}, {1, 1, 1});
  const pluss::Series hx = pluss::make_series({0.5}, {1, 1, 1});

  const auto [output, hidden] = nn::gru(input, hx, params, cfg);

  const double r = 1.0 / (1.0 + std::exp(-0.1));
  const double z = 1.0 / (1.0 + std::exp(-0.2));
  const double n = std::tanh(0.3);
  const double expected = n + z * (0.5 - n);

  std::cout << "output: " << output.item<double>() << "\n";
  std::cout << "hand check: n + z*(h_prev-n) = " << expected << "\n";

  return 0;
}
```

Output

```text
output: 0.406056
hand check: n + z*(h_prev-n) = 0.406056
```

### Example 2

A live [`GRU`](../rnn/GRU.md) instance's `params()` fed directly into
this function matches `forward`'s own output exactly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(3, 4, /*num_layers=*/2);
  const pluss::Series input = pluss::make_series(
      {1.0, 0.0, 0.0,
       0.0, 1.0, 0.0},
      {2, 1, 3});

  const auto [module_output, module_hidden] = net.forward(input);

  nn::rnn::NetConfig cfg{};
  cfg.num_layers = net->options().num_layers;
  cfg.batch_first = net->options().batch_first;

  const auto [func_output, func_hidden] = nn::gru(input, net->params(), cfg);

  std::cout << "identical: " << std::boolalpha << module_output.allclose(func_output) << "\n";

  return 0;
}
```

Output

```text
identical: true
```

### Example 3

`dropout` between layers is deterministic given `cfg.seed` -- two calls
with the same seed produce identical output. `training=false` skips
dropout entirely, regardless of `seed`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(2, 3, /*num_layers=*/2, /*bias=*/true, /*batch_first=*/false, /*dropout=*/0.5);
  net->options().seed = 17;
  net->reset_parameters();

  const pluss::Series input = pluss::make_series({1.0, 0.5, 0.0, 1.0}, {2, 1, 2});

  nn::rnn::NetConfig cfg{};
  cfg.num_layers = 2;
  cfg.dropout = 0.5;
  cfg.seed = 17;
  cfg.training = true;

  const auto [out1, h1] = nn::gru(input, net->params(), cfg);
  const auto [out2, h2] = nn::gru(input, net->params(), cfg);

  std::cout << "same seed, same dropout mask each call -- identical output: "
            << std::boolalpha << out1.allclose(out2) << "\n";

  cfg.training = false;
  const auto [out_eval, h_eval] = nn::gru(input, net->params(), cfg);
  std::cout << "training=false skips dropout entirely (deterministic regardless of seed)\n";

  return 0;
}
```

Output

```text
same seed, same dropout mask each call -- identical output: true
training=false skips dropout entirely (deterministic regardless of seed)
```

### Example 4

`bidirectional=true` requires exactly two [`NetWeight`](NetWeight.md)
blocks.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(2, 3, /*num_layers=*/1, /*bias=*/true, /*batch_first=*/false,
             /*dropout=*/0.0, /*bidirectional=*/true);
  const pluss::Series input = pluss::make_series(
      {1.0, 0.0,
       0.0, 1.0,
       1.0, 1.0},
      {3, 1, 2});

  const auto [module_output, module_hidden] = net.forward(input);

  nn::rnn::NetConfig cfg{};
  cfg.bidirectional = true;

  std::vector<nn::rnn::NetWeight> params_vec(net->params().begin(), net->params().end());
  const auto [func_output, func_hidden] = nn::gru(
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
- [`rnn`](rnn.md) / [`lstm`](lstm.md) -- the plain and doubly-gated
  counterparts, same calling convention.
- [`GRU`](../rnn/GRU.md) -- the stateful `Module` wrapper this function
  powers.
