<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 05f3d58c25c7 -->

# `pluss::ai::nn::RNN::forward`

[Back to `pluss::ai::nn::RNN`](../RNN.md)

## Signature

```cpp
std::tuple<Series, Series> nn::RNN::forward(const Series& input, const Series& hx = Series()) const;
std::tuple<PackedSequence, Series> nn::RNN::forward(const PackedSequence& input, const Series& hx = Series()) const;
```

## What It Does

Runs every layer (and, if `bidirectional`, both directions of every
layer) across the whole sequence, one timestep at a time, per
[`Introduction`](Introduction.md#the-per-timestep-formula)'s formula --
each layer's full output sequence feeds the next layer as input, and
`dropout` (when set and `num_layers > 1`) is applied to every
inter-layer handoff except the last. Returns the final layer's output
sequence alongside every layer/direction's own final hidden state, stacked
together. The `PackedSequence` overload unpacks internally, runs the plain
`Series` overload, and repacks the result -- see Example 4.

## Parameters

`const Series& input`
: `[seq_len, batch, input_size]` (time-major, the default) or `[batch,
  seq_len, input_size]` when `options().batch_first == true`. The last
  axis must equal `options().input_size`.

`const PackedSequence& input`
: A variable-length batch, as produced by
  [`pack_padded_sequence`](../pack_padded_sequence.md). Internally
  unpadded, run, and repacked -- see Example 4.

`const Series& hx`
: The initial hidden state, `[num_layers * dirs, batch, hidden_size]`
  (`dirs` is `2` if `options().bidirectional` else `1`). Defaults to an
  undefined `Series`, which is treated as all-zeros -- there is no
  separate "zero state" sentinel to construct by hand.

## Returns

A `std::tuple<Series, Series>` (or `std::tuple<PackedSequence, Series>`
for the packed overload):

- The output sequence -- same layout convention as `input`
  (time-major/batch-first), with the last axis `hidden_size * dirs` wide
  (every direction's output concatenated along the feature axis when
  bidirectional).
- The final hidden state, `[num_layers * dirs, batch, hidden_size]` --
  the same shape `hx` takes, so it can be fed directly into a later
  `forward` call to continue a sequence across calls.

## Errors

- `hx`'s shape does not match `[num_layers * dirs, batch, hidden_size]`
  for the given `input` -- throws.
- `input`'s last axis does not equal `options().input_size` -- throws.
- `input` has rank less than 3 (for the plain `Series` overload) -- throws.

## Practical Examples

Five examples: a hand-verified single timestep confirming the exact bias
placement, the `batch_first` layout convention verified across three
timesteps, multi-layer bidirectional shapes, a `PackedSequence` round
trip, and confirming `RNN` runs on the MPS backend.

### Example 1

`input_size=1`, `hidden_size=1`, `weight_hh=0` (isolating the input term),
a single timestep -- the result matches `tanh(x*weight_ih + bias)`
exactly, confirming that with the default `split_bias=false`, the bias is
added to the input-side projection only, never doubled onto the
hidden-side one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(1, 1);  // input_size=1, hidden_size=1, num_layers=1 (defaults), nonlinearity=tanh
  net->params()[0].weight_ih.copy_(pluss::make_series({2.0}, {1, 1}));
  net->params()[0].weight_hh.copy_(pluss::make_series({0.0}, {1, 1}));
  net->params()[0].bias.copy_(pluss::make_series({0.5}, {1}));

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  net.to(pluss::ScalarType::Float64);
  // seq_len=1, batch=1, input_size=1 (time-major, the default layout)
  const pluss::Series x = pluss::make_series({0.1}, {1, 1, 1}, f64);
  const auto [output, hidden] = net.forward(x);

  const double expected = std::tanh(0.1 * 2.0 + 0.5);
  std::cout << "output shape: [" << output.sizes()[0] << "," << output.sizes()[1] << "," << output.sizes()[2] << "]\n";
  std::cout << "hidden shape: [" << hidden.sizes()[0] << "," << hidden.sizes()[1] << "," << hidden.sizes()[2] << "]\n";
  std::cout << "output value: " << output << "\n";
  std::cout << "expected tanh(0.1*2 + 0.5) = " << expected << "\n";
  std::cout << "output equals final hidden (only 1 timestep): " << std::boolalpha
            << output.reshape({1}).allclose(hidden.reshape({1})) << "\n";

  return 0;
}
```

Output

```text
output shape: [1,1,1]
hidden shape: [1,1,1]
output value: pluss::Series(shape=[1,1,1], strides=[1,1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[[0.60436778]]]
expected tanh(0.1*2 + 0.5) = 0.604368
output equals final hidden (only 1 timestep): true
```

### Example 2

`batch_first=true` over three timesteps, verified against the same
recurrence run by hand: `h_0=tanh(1)`, `h_1=tanh(1+0.5*h_0)`,
`h_2=tanh(1+0.5*h_1)`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNNOptions opts{};
  opts.input_size = 1;
  opts.hidden_size = 1;
  opts.batch_first = true;
  nn::RNN net(opts);
  net->params()[0].weight_ih.copy_(pluss::make_series({1.0}, {1, 1}));
  net->params()[0].weight_hh.copy_(pluss::make_series({0.5}, {1, 1}));
  net->params()[0].bias.copy_(pluss::make_series({0.0}, {1}));

  net.to(pluss::ScalarType::Float64);
  // batch_first=true -- input is [batch=1, seq_len=3, input_size=1]
  const pluss::Series x = pluss::make_series({1.0, 1.0, 1.0}, {1, 3, 1}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto [output, hidden] = net.forward(x);

  std::cout << "batch_first=true: output shape: [" << output.sizes()[0] << "," << output.sizes()[1]
            << "," << output.sizes()[2] << "] -- [batch, seq_len, hidden_size]\n";
  std::cout << "output: " << output << "\n";
  std::cout << "final hidden: " << hidden << "\n";

  return 0;
}
```

Output

```text
batch_first=true: output shape: [1,3,1] -- [batch, seq_len, hidden_size]
output: pluss::Series(shape=[1,3,1], strides=[1,1,1], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[[[0.76159416],
  [0.88112963],
  [0.89381137]]]
final hidden: pluss::Series(shape=[1,1,1], strides=[1,1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[[0.89381137]]]
```

### Example 3

`num_layers=2`, `bidirectional=true` -- `params()` holds 4 weight blocks
(`layers(2) * dirs(2)`), the output's feature axis is `hidden_size * dirs`
wide, and the hidden state's leading axis is `num_layers * dirs`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNNOptions opts{};
  opts.input_size = 4;
  opts.hidden_size = 8;
  opts.num_layers = 2;
  opts.bidirectional = true;
  nn::RNN net(opts);
  net.to(pluss::ScalarType::Float64);

  std::cout << "params().size() = " << net->params().size()
            << " -- num_layers(2) * dirs(2) = 4 weight blocks\n";

  // seq_len=5, batch=3, input_size=4 (time-major, default layout)
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
output shape: [5,3,16] -- [seq_len, batch, hidden_size*dirs]
hidden shape: [4,3,8] -- [num_layers*dirs, batch, hidden_size]
```

### Example 4

A 2-sequence batch with lengths `{3, 1}`, packed via
[`pack_padded_sequence`](../pack_padded_sequence.md), run through the
`PackedSequence` overload, and unpacked again via
[`pad_packed_sequence`](../pad_packed_sequence.md) -- the shorter
sequence's padded positions never influence the longer sequence's
computation, since the packed form only ever holds real timesteps.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::RNN net(1, 2);  // input_size=1, hidden_size=2
  net.to(pluss::ScalarType::Float64);

  const pluss::Config f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  // batch=2, seq_len=3, input_size=1 -- sequence 0 has length 3, sequence 1 has length 1
  // (padded positions after each sequence's real length are exactly 0)
  const pluss::Series padded_input = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 0.0, 0.0}, {2, 3, 1}, f64);
  const pluss::Series lengths = pluss::make_series({3.0, 1.0}, {2}).to(pluss::ScalarType::Int64);

  const nn::PackedSequence packed = nn::pack_padded_sequence(padded_input, lengths, /*batch_first=*/true);
  std::cout << "packed.data numel: " << packed.data.numel() << " -- 3+1=4 real timesteps, not 2*3=6\n";

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
packed.data numel: 4 -- 3+1=4 real timesteps, not 2*3=6
restored output shape: [2,3,2]
restored lengths: pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[3, 1]
```

### Example 5

`RNN` runs on the MPS backend the same way it does on CPU.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::RNN net(1, 2);
  net.to(pluss::pMPS);

  std::cout << "weight_ih platform after .to(pMPS): " << static_cast<int>(net->params()[0].weight_ih.platform()) << "\n";

  const Series x = pluss::make_series({1.0, 2.0, 3.0}, {3, 1, 1}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const auto [output, hidden] = net.forward(x);
    std::cout << "rnn on MPS: OK, platform=" << static_cast<int>(output.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "rnn on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
weight_ih platform after .to(pMPS): 2
rnn on MPS: OK, platform=2
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
