# `pluss::ai::nn::rnn::NetConfig`

The full configuration `pluss::ai::nn::rnn` / `gru` / `lstm` read:
layout, layers, dropout, bidirectionality, projection size.

## Signature

```cpp
bool training;
bool batch_first;
bool bidirectional;
double dropout;
std::int64_t num_layers;
bool use_bias;
bool split_bias;
bool pre_compute_input;
std::int64_t proj_size;
std::string nonlinearity;
pluss::Config cfg;
std::uint64_t seed;
```

A plain aggregate -- there is no constructor beyond the implicit
default/aggregate ones. Passed to [`rnn`](rnn.md)/[`gru`](gru.md)/
[`lstm`](lstm.md)'s trailing `cfg` argument, defaulted, replacing every
geometry/behavior field a constructed `RNN`/`GRU`/`LSTM`
[`Options`](../rnn/RNNOptions.md) struct would otherwise supply, since
there is no `Module` instance to read them from in the functional form.

## State

`training` -- `bool`, default `true`
: Whether `dropout` (between layers) is active.

`batch_first` -- `bool`, default `false`
: Which axis of a batched `input` is the sequence-length axis.

`bidirectional` -- `bool`, default `false`
: Whether each layer also runs over the time-reversed sequence. Doubles
  the required `params` block count per layer (`num_layers * 2`).

`dropout` -- `double`, default `0.0`
: Applied between layers only (never after the last layer, never when
  `num_layers == 1`), seeded from `seed` (offset per layer).

`num_layers` -- `std::int64_t`, default `1`
: How many recurrent layers `params` must supply blocks for. Must be
  `> 0`.

`use_bias` -- `bool`, default `true`
: Whether every `params` block is expected to carry a bias (unified or
  split, per `split_bias`). Mismatching this against what `params`
  actually carries throws.

`split_bias` -- `bool`, default `false`
: Whether bias lives in `bias` (unified, `false`) or `bias_ih`/`bias_hh`
  (split, `true`) on each `params` block.

`pre_compute_input` -- `bool`, default `true`
: A performance path choice only -- whether the input-side projection is
  computed once for the whole sequence up front (`true`) or recomputed
  per timestep (`false`). Produces bit-for-bit identical results either
  way; see Example 2.

`proj_size` -- `std::int64_t`, default `0`
: For [`lstm`](lstm.md) only -- `> 0` activates the recurrent
  projection, reading each `params` block's `weight_hr`. Ignored by
  [`rnn`](rnn.md)/[`gru`](gru.md).

`nonlinearity` -- `std::string`, default `"tanh"`
: For [`rnn`](rnn.md) only -- `"tanh"` or `"relu"`, nothing else
  (anything else throws). Ignored by [`gru`](gru.md)/[`lstm`](lstm.md),
  whose gate nonlinearities are fixed by their own architecture.

`cfg` -- `pluss::Config`, default engine default
: Present for symmetry with the `Module` classes' own `Options`
  structs, but **not consulted** by `rnn`/`gru`/`lstm` themselves -- the
  zero-filled initial hidden/cell state (when `hx`/`state` is omitted)
  is built to match `input`'s own dtype/platform directly, not this
  field. See Example 3.

`seed` -- `std::uint64_t`, default `0`
: The base seed inter-layer `dropout` draws from, offset per layer.

## Practical Examples

Three examples: reading back every field's default value plus an
invalid-`nonlinearity` error, `pre_compute_input` producing identical
results either way, and confirming `cfg` genuinely has no effect on the
functional entry points' own output.

### Example 1

Every field's default value, and confirmation that an invalid
`nonlinearity` (only `"tanh"`/`"relu"` are accepted) throws at call
time, not at construction.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <string>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetConfig cfg{};
  std::cout << "defaults: training=" << std::boolalpha << cfg.training
            << ", batch_first=" << cfg.batch_first
            << ", bidirectional=" << cfg.bidirectional
            << ", dropout=" << cfg.dropout
            << ", num_layers=" << cfg.num_layers
            << ", use_bias=" << cfg.use_bias
            << ", split_bias=" << cfg.split_bias
            << ", proj_size=" << cfg.proj_size
            << ", nonlinearity=" << cfg.nonlinearity << "\n";

  // an invalid nonlinearity is rejected at call time, not at construction
  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.5}, {1, 1});
  params.weight_hh = pluss::make_series({0.0}, {1, 1});
  params.bias = pluss::make_series({0.0}, {1});

  nn::rnn::NetConfig bad_cfg{};
  bad_cfg.nonlinearity = "sigmoid";  // only "tanh"/"relu" are valid
  const pluss::Series input = pluss::make_series({1.0}, {1, 1, 1});

  try {
    const auto [output, hidden] = nn::rnn(input, params, bad_cfg);
  } catch (const std::exception& e) {
    const std::string message(e.what());
    std::cout << "invalid nonlinearity throws: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
defaults: training=true, batch_first=false, bidirectional=false, dropout=0, num_layers=1, use_bias=true, split_bias=false, proj_size=0, nonlinearity=tanh
invalid nonlinearity throws: pluss::rnn: nonlinearity must be 'tanh' or 'relu'
```

### Example 2

`pre_compute_input=true` and `false` compute the exact same result --
purely a performance-path choice, never a semantic one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.3, -0.2}, {1, 2});
  params.weight_hh = pluss::make_series({0.1}, {1, 1});
  params.bias = pluss::make_series({0.05}, {1});

  const pluss::Series input = pluss::make_series({1.0, 0.5, 0.0, 1.0}, {2, 1, 2});

  nn::rnn::NetConfig precompute_cfg{};
  precompute_cfg.pre_compute_input = true;
  const auto [out_pre, h_pre] = nn::rnn(input, params, precompute_cfg);

  nn::rnn::NetConfig stepwise_cfg{};
  stepwise_cfg.pre_compute_input = false;
  const auto [out_step, h_step] = nn::rnn(input, params, stepwise_cfg);

  std::cout << "pre_compute_input=true and false produce identical results (a performance path, not a semantic one): "
            << std::boolalpha << out_pre.allclose(out_step) << "\n";

  return 0;
}
```

Output

```text
pre_compute_input=true and false produce identical results (a performance path, not a semantic one): true
```

### Example 3

Setting `cfg.cfg` to `Float32` while `params`/`input` are `Float64` has
no effect at all -- the output stays `Float64`, following `input`'s own
dtype, confirming `cfg.cfg` is not consulted by these functional entry
points.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::rnn::NetWeight params{};
  params.weight_ih = pluss::make_series({0.5}, {1, 1}).to(pluss::ScalarType::Float64);
  params.weight_hh = pluss::make_series({0.0}, {1, 1}).to(pluss::ScalarType::Float64);
  params.bias = pluss::make_series({0.0}, {1}).to(pluss::ScalarType::Float64);

  const pluss::Series input = pluss::make_series({2.0}, {1, 1, 1}).to(pluss::ScalarType::Float64);

  // cfg.cfg set to Float32, deliberately mismatched from the Float64
  // weights/input above -- does it affect anything here?
  nn::rnn::NetConfig cfg{};
  cfg.cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32);

  const auto [output, hidden] = nn::rnn(input, params, cfg);

  std::cout << "output dtype: " << static_cast<int>(output.scalar_type())
            << " (Float64=" << static_cast<int>(pluss::ScalarType::Float64)
            << ", Float32=" << static_cast<int>(pluss::ScalarType::Float32) << ")\n";
  std::cout << "output value: " << output.item<double>()
            << " -- cfg.cfg's dtype has no effect here; the zero initial hidden "
               "state (and everything else) follows input's own Float64 dtype\n";

  return 0;
}
```

Output

```text
output dtype: 12 (Float64=12, Float32=11)
output value: 0.761594 -- cfg.cfg's dtype has no effect here; the zero initial hidden state (and everything else) follows input's own Float64 dtype
```

## Methods

None -- a plain configuration aggregate; every operation that consumes
it is a free function ([`rnn`](rnn.md)/[`gru`](gru.md)/[`lstm`](lstm.md)),
never a member of this type itself.

## Changelog

- **0.1** — the full functional-RNN configuration struct.

## See Also

- [`NetWeight`](NetWeight.md) -- the companion explicit-weight struct.
- [`rnn`](rnn.md) / [`gru`](gru.md) / [`lstm`](lstm.md) -- the three
  consumers of this type.
- [`RNNOptions`](../rnn/RNNOptions.md) /
  [`GRUOptions`](../rnn/GRUOptions.md) /
  [`LSTMOptions`](../rnn/LSTMOptions.md) -- the analogous `Options`
  structs for the stateful `Module` classes, where `cfg` (there, the
  field driving actual parameter allocation) is genuinely consulted,
  unlike here.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
