<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0802a58b8944 -->

# `pluss::ai::nn::GRU::forward`

[Back to `pluss::ai::nn::GRU`](../GRU.md)

## Signature

```cpp
std::tuple<Series, Series> nn::GRU::forward(const Series& input, const Series& hx = Series()) const;
std::tuple<PackedSequence, Series> nn::GRU::forward(const PackedSequence& input, const Series& hx = Series()) const;
```

## What It Does

Runs every layer (and, if `bidirectional`, both directions of every
layer) across the whole sequence, one timestep at a time, per
[`Introduction`](Introduction.md#the-per-timestep-formula)'s gate
formulas -- each layer's full output sequence feeds the next layer as
input, and `dropout` (when set and `num_layers > 1`) is applied to every
inter-layer handoff except the last. Returns the final layer's output
sequence alongside every layer/direction's own final hidden state. The
`PackedSequence` overload unpacks internally, runs the plain `Series`
overload, and repacks the result -- see Example 4.

## Parameters

`const Series& input`
: `[seq_len, batch, input_size]` (time-major, the default) or `[batch,
  seq_len, input_size]` when `options().batch_first == true`.

`const PackedSequence& input`
: A variable-length batch, as produced by
  [`pack_padded_sequence`](../pack_padded_sequence.md).

`const Series& hx`
: The initial hidden state, `[num_layers * dirs, batch, hidden_size]`.
  Defaults to an undefined `Series`, treated as all-zeros.

## Returns

A `std::tuple<Series, Series>` (or `std::tuple<PackedSequence, Series>`
for the packed overload):

- The output sequence -- same layout convention as `input`, last axis
  `hidden_size * dirs` wide.
- The final hidden state, `[num_layers * dirs, batch, hidden_size]` --
  the same shape `hx` takes.

## Errors

- `hx`'s shape does not match `[num_layers * dirs, batch, hidden_size]`
  -- throws.
- `input`'s last axis does not equal `options().input_size` -- throws.
- `input` has rank less than 3 (for the plain `Series` overload) --
  throws.

## Practical Examples

Five examples: a hand-verified single timestep confirming the exact gate
formula, `batch_first` shapes with the last output timestep matching the
final hidden state, multi-layer bidirectional shapes, a `PackedSequence`
round trip, and confirming `GRU` runs on the MPS backend.

### Example 1

`input_size=1`, `hidden_size=1`, `weight_hh=0` (isolating the input
term) -- the result matches `n*(1-z)` computed independently, confirming
the reset/update/candidate gate order and the exact blend formula from
[`Introduction`](../Introduction.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(1, 1);  // input_size=1, hidden_size=1
  // weight_ih/weight_hh are [3*hidden_size, ...] -- gate order is (r, z, n)
  net->params()[0].weight_ih.copy_(pluss::make_series({1.0, 1.0, 1.0}, {3, 1}));
  net->params()[0].weight_hh.copy_(pluss::make_series({0.0, 0.0, 0.0}, {3, 1}));
  net->params()[0].bias.copy_(pluss::make_series({0.0, 0.0, 0.0}, {3}));

  net.to(pluss::ScalarType::Float64);
  const pluss::Series x = pluss::make_series({1.0}, {1, 1, 1}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto [output, hidden] = net.forward(x);

  const double r = 1.0 / (1.0 + std::exp(-1.0));
  const double z = r;  // same formula, same inputs
  const double n = std::tanh(1.0);
  const double expected = n * (1.0 - z);  // h' = n + z*(h-n), h=0

  std::cout << "output: " << output << "\n";
  std::cout << "expected n*(1-z) = " << expected << "\n";
  std::cout << "matches: " << std::boolalpha << output.reshape({1}).allclose(hidden.reshape({1})) << "\n";

  return 0;
}
```

Output

```text
output: pluss::Series(shape=[1,1,1], strides=[1,1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[[0.20482421]]]
expected n*(1-z) = 0.204824
matches: true
```

### Example 2

`batch_first=true` over four timesteps -- the last output timestep always
equals the final hidden state, the same relationship
[`RNN::forward`](../RNN/forward.md) shows.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRUOptions opts{};
  opts.input_size = 2;
  opts.hidden_size = 3;
  opts.batch_first = true;
  nn::GRU net(opts);
  net.to(pluss::ScalarType::Float64);

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // batch_first=true -- [batch=2, seq_len=4, input_size=2]
  const pluss::Series x = pluss::Series::full({2, 4, 2}, 0.3, pluss::ScalarType::Float64);
  const auto [output, hidden] = net.forward(x);

  std::cout << "output shape: [" << output.sizes()[0] << "," << output.sizes()[1] << "," << output.sizes()[2]
            << "] -- [batch, seq_len, hidden_size]\n";
  std::cout << "hidden shape: [" << hidden.sizes()[0] << "," << hidden.sizes()[1] << "," << hidden.sizes()[2]
            << "] -- [num_layers*dirs, batch, hidden_size]\n";
  std::cout << "last output timestep equals final hidden: " << std::boolalpha
            << output.select(1, 3).select(0, 0).allclose(hidden.select(0, 0).select(0, 0)) << "\n";

  return 0;
}
```

Output

```text
output shape: [2,4,3] -- [batch, seq_len, hidden_size]
hidden shape: [1,2,3] -- [num_layers*dirs, batch, hidden_size]
last output timestep equals final hidden: true
```

### Example 3

`num_layers=2`, `bidirectional=true` -- `params()` holds 4 weight blocks,
each `weight_ih` shaped `[3*hidden_size, ...]`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRUOptions opts{};
  opts.input_size = 4;
  opts.hidden_size = 8;
  opts.num_layers = 2;
  opts.bidirectional = true;
  nn::GRU net(opts);
  net.to(pluss::ScalarType::Float64);

  std::cout << "params().size() = " << net->params().size() << " -- num_layers(2) * dirs(2) = 4 weight blocks\n";
  std::cout << "params()[0].weight_ih shape: [" << net->params()[0].weight_ih.sizes()[0] << ","
            << net->params()[0].weight_ih.sizes()[1] << "] -- [3*hidden_size, input_size]\n";

  const pluss::Series x = pluss::Series::full({5, 3, 4}, 0.1, pluss::ScalarType::Float64);
  const auto [output, hidden] = net.forward(x);

  std::cout << "output shape: [" << output.sizes()[0] << "," << output.sizes()[1] << "," << output.sizes()[2]
            << "] -- [seq_len, batch, hidden_size*dirs]\n";
  std::cout << "hidden shape: [" << hidden.sizes()[0] << "," << hidden.sizes()[1] << "," << hidden.sizes()[2]
            << "] -- [num_layers*dirs, batch, hidden_size]\n";

  return 0;
}
```

Output

```text
params().size() = 4 -- num_layers(2) * dirs(2) = 4 weight blocks
params()[0].weight_ih shape: [24,4] -- [3*hidden_size, input_size]
output shape: [5,3,16] -- [seq_len, batch, hidden_size*dirs]
hidden shape: [4,3,8] -- [num_layers*dirs, batch, hidden_size]
```

### Example 4

A 2-sequence batch with lengths `{3, 1}`, packed, run through the
`PackedSequence` overload, and unpacked again -- the same round trip
[`RNN::forward`](../RNN/forward.md) shows, on a `GRU` instead.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::GRU net(1, 2);
  net.to(pluss::ScalarType::Float64);

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series padded_input = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 0.0, 0.0}, {2, 3, 1}, f64);
  const pluss::Series lengths = pluss::make_series({3.0, 1.0}, {2}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded_input, lengths, /*batch_first=*/true);
  const auto [packed_output, hidden] = net.forward(packed);
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

`GRU` runs on the MPS backend the same way it does on CPU.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::GRU net(1, 2);
  net.to(pluss::pMPS);

  std::cout << "weight_ih platform after .to(pMPS): " << static_cast<int>(net->params()[0].weight_ih.platform()) << "\n";

  const Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1, 1}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const auto [output, hidden] = net.forward(x);
    std::cout << "gru on MPS: OK, platform=" << static_cast<int>(output.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "gru on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight_ih platform after .to(pMPS): 2
gru on MPS: OK, platform=2
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
