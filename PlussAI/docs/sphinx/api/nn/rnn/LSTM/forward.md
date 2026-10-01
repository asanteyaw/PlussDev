<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 892dd938ef48 -->

# `pluss::ai::nn::LSTM::forward`

[Back to `pluss::ai::nn::LSTM`](../LSTM.md)

## Signature

```cpp
std::tuple<Series, std::tuple<Series, Series>> nn::LSTM::forward(
    const Series& input, const std::tuple<Series, Series>& state = {}) const;
std::tuple<PackedSequence, std::tuple<Series, Series>> nn::LSTM::forward(
    const PackedSequence& input, const std::tuple<Series, Series>& state = {}) const;
```

## What It Does

Runs every layer (and, if `bidirectional`, both directions of every
layer) across the whole sequence, one timestep at a time, per
[`Introduction`](Introduction.md#the-per-timestep-formula)'s gate
formulas -- maintaining both a hidden state and a separate cell state
throughout. Each layer's full output sequence feeds the next layer as
input, and `dropout` (when set and `num_layers > 1`) is applied to every
inter-layer handoff except the last. Returns the final layer's output
sequence alongside every layer/direction's own final `(hidden, cell)`
state pair. The `PackedSequence` overload unpacks internally, runs the
plain `Series` overload, and repacks the result.

## Parameters

`const Series& input`
: `[seq_len, batch, input_size]` (time-major, the default) or `[batch,
  seq_len, input_size]` when `options().batch_first == true`.

`const PackedSequence& input`
: A variable-length batch, as produced by
  [`pack_padded_sequence`](../pack_padded_sequence.md).

`const std::tuple<Series, Series>& state`
: The initial `(hidden, cell)` pair. `hidden` is `[num_layers * dirs,
  batch, proj_size_or_hidden_size]`; `cell` is always `[num_layers *
  dirs, batch, hidden_size]`, regardless of `proj_size`. Either or both
  elements may be an undefined `Series` (the default, an empty tuple),
  treated as all-zeros independently -- you can supply a real `hidden`
  while leaving `cell` to default, or vice versa.

## Returns

A `std::tuple<Series, std::tuple<Series, Series>>` (or with
`PackedSequence` in place of the first `Series` for the packed overload):

- The output sequence -- same layout convention as `input`, last axis
  `(proj_size or hidden_size) * dirs` wide.
- The final `(hidden, cell)` pair, the same shapes `state`'s two elements
  take.

## Errors

- `state`'s `hidden`/`cell` shapes do not match what `input` and
  `options()` imply -- throws.
- `input`'s last axis does not equal `options().input_size` -- throws.
- `input` has rank less than 3 (for the plain `Series` overload) --
  throws.

## Practical Examples

Five examples: a hand-verified single timestep confirming the exact gate
order and cell-state update, `proj_size` changing the hidden/output width
while leaving the cell state untouched, multi-layer bidirectional shapes,
a `PackedSequence` round trip, and confirming `LSTM` runs on the MPS
backend.

### Example 1

`input_size=1`, `hidden_size=1`, `weight_hh=0` (isolating the input
term), initial cell `0` -- the result matches `o*tanh(i*g)` computed
independently, confirming the `(input, forget, cell, output)` gate order
and the cell-state update formula from
[`Introduction`](../Introduction.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(1, 1);  // input_size=1, hidden_size=1
  // weight_ih/weight_hh are [4*hidden_size, ...] -- gate order is (i, f, g, o)
  net->params()[0].weight_ih.copy_(pluss::make_series({1.0, 1.0, 1.0, 1.0}, {4, 1}));
  net->params()[0].weight_hh.copy_(pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4, 1}));
  net->params()[0].bias.copy_(pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4}));

  net.to(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0}, {1, 1, 1}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto [output, state] = net.forward(x);
  const auto [hidden, cell] = state;

  const double i = 1.0 / (1.0 + std::exp(-1.0));
  const double f = i;
  const double g = std::tanh(1.0);
  const double o = i;
  const double expected_c = f * 0.0 + i * g;  // initial cell = 0
  const double expected_h = o * std::tanh(expected_c);

  std::cout << "output: " << output << "\n";
  std::cout << "cell: " << cell << "\n";
  std::cout << "expected h = o*tanh(i*g) = " << expected_h << "\n";
  std::cout << "expected c = i*g = " << expected_c << "\n";
  std::cout << "h matches: " << std::boolalpha << output.reshape({1}).allclose(hidden.reshape({1})) << "\n";

  return 0;
}
```

Output

```text
output: pluss::Series(shape=[1,1,1], strides=[1,1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[[0.36960635]]]
cell: pluss::Series(shape=[1,1,1], strides=[1,1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[[0.55676994]]]
expected h = o*tanh(i*g) = 0.369606
expected c = i*g = 0.55677
h matches: true
```

### Example 2

`proj_size=3` on `hidden_size=8` -- the output and hidden state both
become `proj_size`-wide, but the cell state stays `hidden_size`-wide,
since only the hidden/output side of an `LSTM` is ever projected.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTMOptions opts{};
  opts.input_size = 4;
  opts.hidden_size = 8;
  opts.proj_size = 3;  // project the recurrent/output hidden state down to width 3
  nn::LSTM net(opts);
  net.to(pluss::ScalarType::Float64);

  std::cout << "params()[0].weight_hr defined: " << std::boolalpha << net->params()[0].weight_hr.defined() << "\n";
  std::cout << "params()[0].weight_hr shape: [" << net->params()[0].weight_hr.sizes()[0] << ","
            << net->params()[0].weight_hr.sizes()[1] << "] -- [proj_size, hidden_size]\n";

  const pluss::Series x = pluss::Series::full({3, 2, 4}, 0.2, pluss::ScalarType::Float64);
  const auto [output, state] = net.forward(x);
  const auto [hidden, cell] = state;

  std::cout << "output shape: [" << output.sizes()[0] << "," << output.sizes()[1] << "," << output.sizes()[2]
            << "] -- last axis is proj_size, not hidden_size\n";
  std::cout << "hidden shape: [" << hidden.sizes()[0] << "," << hidden.sizes()[1] << "," << hidden.sizes()[2]
            << "] -- also proj_size-wide (the recurrent/output state)\n";
  std::cout << "cell shape: [" << cell.sizes()[0] << "," << cell.sizes()[1] << "," << cell.sizes()[2]
            << "] -- still hidden_size-wide (the cell state is never projected)\n";

  return 0;
}
```

Output

```text
params()[0].weight_hr defined: true
params()[0].weight_hr shape: [3,8] -- [proj_size, hidden_size]
output shape: [3,2,3] -- last axis is proj_size, not hidden_size
hidden shape: [1,2,3] -- also proj_size-wide (the recurrent/output state)
cell shape: [1,2,8] -- still hidden_size-wide (the cell state is never projected)
```

This exercises a real fix made alongside this documentation: `weight_hr`
used to be allocated `[proj_size, proj_size]` instead of `[proj_size,
hidden_size]`, which made the projection matmul in `forward` fail its own
shape check the moment `proj_size` was actually set to a value different
from `hidden_size`. It is now sized correctly.

### Example 3

`num_layers=2`, `bidirectional=true`, no projection -- `params()` holds 4
weight blocks, and (with `proj_size == 0`) `hidden`/`cell` share the same
shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTMOptions opts{};
  opts.input_size = 4;
  opts.hidden_size = 8;
  opts.num_layers = 2;
  opts.bidirectional = true;
  nn::LSTM net(opts);
  net.to(pluss::ScalarType::Float64);

  std::cout << "params().size() = " << net->params().size() << " -- num_layers(2) * dirs(2) = 4 weight blocks\n";
  std::cout << "params()[0].weight_ih shape: [" << net->params()[0].weight_ih.sizes()[0] << ","
            << net->params()[0].weight_ih.sizes()[1] << "] -- [4*hidden_size, input_size]\n";

  const pluss::Series x = pluss::Series::full({5, 3, 4}, 0.1, pluss::ScalarType::Float64);
  const auto [output, state] = net.forward(x);
  const auto [hidden, cell] = state;

  std::cout << "output shape: [" << output.sizes()[0] << "," << output.sizes()[1] << "," << output.sizes()[2]
            << "] -- [seq_len, batch, hidden_size*dirs]\n";
  std::cout << "hidden shape: [" << hidden.sizes()[0] << "," << hidden.sizes()[1] << "," << hidden.sizes()[2]
            << "] -- [num_layers*dirs, batch, hidden_size]\n";
  std::cout << "cell shape: [" << cell.sizes()[0] << "," << cell.sizes()[1] << "," << cell.sizes()[2]
            << "] -- same shape as hidden here (no projection)\n";

  return 0;
}
```

Output

```text
params().size() = 4 -- num_layers(2) * dirs(2) = 4 weight blocks
params()[0].weight_ih shape: [32,4] -- [4*hidden_size, input_size]
output shape: [5,3,16] -- [seq_len, batch, hidden_size*dirs]
hidden shape: [4,3,8] -- [num_layers*dirs, batch, hidden_size]
cell shape: [4,3,8] -- same shape as hidden here (no projection)
```

### Example 4

A 2-sequence batch with lengths `{3, 1}`, packed, run through the
`PackedSequence` overload, and unpacked again.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::LSTM net(1, 2);
  net.to(pluss::ScalarType::Float64);

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series padded_input = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 0.0, 0.0}, {2, 3, 1}, f64);
  const pluss::Series lengths = pluss::make_series({3.0, 1.0}, {2}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded_input, lengths, /*batch_first=*/true);
  const auto [packed_output, state] = net.forward(packed);
  const auto [restored_output, restored_lengths] = nn::pad_packed_sequence(packed_output, /*batch_first=*/true);

  std::cout << "restored output shape: [" << restored_output.sizes()[0] << "," << restored_output.sizes()[1]
            << "," << restored_output.sizes()[2] << "]\n";
  std::cout << "restored lengths: " << restored_lengths << "\n";

  return 0;
}
```

Output

```text
restored output shape: [2,3,2]
restored lengths: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[3, 1]
```

### Example 5

`LSTM` runs on the MPS backend the same way it does on CPU.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::LSTM net(1, 2);
  net.to(pluss::pMPS);

  std::cout << "weight_ih platform after .to(pMPS): " << static_cast<int>(net->params()[0].weight_ih.platform()) << "\n";

  const Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1, 1}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const auto [output, state] = net.forward(x);
    std::cout << "lstm on MPS: OK, platform=" << static_cast<int>(output.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "lstm on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight_ih platform after .to(pMPS): 2
lstm on MPS: OK, platform=2
```

## See Also

- [`params`](params.md) -- the weight blocks `forward` reads.
- [`pack_padded_sequence`](../pack_padded_sequence.md) / [`pad_packed_sequence`](../pad_packed_sequence.md) --
  build and unpack the `PackedSequence` form.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="options.html">
    <span class="label">Next</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
