# `pluss::ai::nn::TransformerDecoder`

A stack of [`TransformerDecoderLayer`](TransformerDecoderLayer.md)
blocks, `num_layers` deep, sharing every option except a per-layer random
seed offset, with an optional final layer normalization applied once
after every layer has run. `forward` feeds `tgt` through layer 0 (which
also attends to the same `memory` every other layer attends to), layer
0's output into layer 1 as the next layer's own `tgt`, and so on --
`memory` itself is never transformed by this class; every layer reads the
identical `memory` passed to this class's own `forward`.

## Signature

```cpp
explicit nn::TransformerDecoder::TransformerDecoder(nn::TransformerDecoderOptions options);
nn::TransformerDecoder::TransformerDecoder(
    std::int64_t d_model, std::int64_t nhead, std::int64_t num_layers = 6,
    std::int64_t dim_feedforward = 2048, double dropout = 0.1,
    nn::TransformerActivation activation = nn::TransformerActivation::ReLU,
    bool batch_first = false, bool norm_first = false, bool bias = true,
    pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::TransformerDecoder::reset_parameters();
```

## Construction

Two equivalent forms: pass a
[`TransformerDecoderOptions`](TransformerDecoderOptions.md) directly, or
spell out the most-used fields as positional arguments -- field-for-field
identical to [`TransformerEncoder`](TransformerEncoder.md#construction)'s
own two forms:

```cpp
nn::TransformerDecoder a(nn::TransformerDecoderOptions{512, 8});  // d_model=512, nhead=8, num_layers=6 (default), everything else defaulted
nn::TransformerDecoder b(512, 8);                                  // equivalent positional form
```

## State

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the [`TransformerDecoderOptions`](TransformerDecoderOptions.md)
this instance was constructed with.

`depth()` (`const`) -- `std::size_t`
: The number of internal `TransformerDecoderLayer` instances -- always
  exactly `options().num_layers`.

`layer(index)` (mutable and `const` overloads) -- `TransformerDecoderLayer&` /
`const TransformerDecoderLayer&`
: The internal layer at `index` (`0 <= index < depth()`), in the order
  `forward` runs them. Throws if `index >= depth()`.

## Methods

`forward(tgt, memory, tgt_mask = {}, memory_mask = {}, tgt_key_padding_mask = {}, memory_key_padding_mask = {}, tgt_is_causal = false, memory_is_causal = false)` /
`operator()(...)` -- `Series`
: Runs `tgt` through every internal layer in order -- each layer both
  self-attends over its own running `tgt` and cross-attends to the same
  `memory` -- then through the final layer normalization when
  `options().final_norm == true`. Same shape as `tgt`. Every mask/causal
  argument is forwarded identically to every layer's own
  [`forward`](TransformerDecoderLayer.md#methods).

`reset_parameters()` -- `void`
: Re-delegates to every internal layer's own `reset_parameters()` (each
  in place), then, when `options().final_norm == true`, re-fills this
  class's own final normalization weight/bias pair in place.

## Practical Examples

Five examples: a 3-layer stack fed a `memory` of a different sequence
length than `tgt`, `final_norm=true` versus `false`, the
positional-vs-options-struct construction equivalence, confirming
`reset_parameters()`'s reproducibility, and confirming the stack runs on
the MPS backend.

### Example 1

A 3-layer stack preserves `tgt`'s shape even though `memory` has a
different sequence length (`3` versus `tgt`'s `2`) -- every layer's
cross-attention handles that independently, the same way
[`TransformerDecoderLayer`](TransformerDecoderLayer.md#practical-examples)
does on its own.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoder decoder(4, 2, /*num_layers=*/3, /*dim_feedforward=*/8, /*dropout=*/0.0);

  std::cout << "depth()=" << decoder->depth() << "\n";

  const pluss::Series tgt = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 1, 4});
  const pluss::Series memory = pluss::make_series(
      {0.5, 0.5, 0.0, 0.0,
       0.0, 0.5, 0.5, 0.0,
       0.0, 0.0, 0.5, 0.5},
      {3, 1, 4});

  const pluss::Series out = decoder.forward(tgt, memory);
  std::cout << "output shape: [" << out.sizes()[0] << ", " << out.sizes()[1] << ", " << out.sizes()[2] << "]\n";

  const bool same_weight = decoder->layer(0)->linear1()->weight().allclose(decoder->layer(1)->linear1()->weight());
  std::cout << "layer(0) and layer(1) linear1 weights identical: " << std::boolalpha << same_weight << "\n";

  return 0;
}
```

Output

```text
depth()=3
output shape: [2, 1, 4]
layer(0) and layer(1) linear1 weights identical: false
```

### Example 2

`final_norm=false` registers no final normalization parameters at
all -- exactly 2 fewer entries in `parameters()` than the `true`
(default) case.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoderOptions with_norm{};
  with_norm.d_model = 4;
  with_norm.nhead = 2;
  with_norm.num_layers = 2;
  with_norm.dim_feedforward = 8;
  with_norm.dropout = 0.0;
  with_norm.final_norm = true;
  nn::TransformerDecoder decoder_with_norm(with_norm);

  nn::TransformerDecoderOptions without_norm = with_norm;
  without_norm.final_norm = false;
  nn::TransformerDecoder decoder_without_norm(without_norm);

  std::cout << "with final_norm: parameters().size()=" << decoder_with_norm.parameters().size() << "\n";
  std::cout << "without final_norm: parameters().size()=" << decoder_without_norm.parameters().size()
            << " -- exactly 2 fewer (no norm_weight/norm_bias)\n";

  const pluss::Series tgt = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}, {2, 1, 4});
  const pluss::Series memory = pluss::make_series({0.5, 0.5, 0.0, 0.0, 0.0, 0.5, 0.5, 0.0}, {2, 1, 4});

  const pluss::Series out_with = decoder_with_norm.forward(tgt, memory);
  std::cout << "with final_norm, output row means (should be ~0): "
            << out_with.reshape({2, 4}).mean(-1, false) << "\n";

  return 0;
}
```

Output

```text
with final_norm: parameters().size()=54
without final_norm: parameters().size()=52 -- exactly 2 fewer (no norm_weight/norm_bias)
with final_norm, output row means (should be ~0): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, -5.5511151e-17]
```

### Example 3

The positional and `TransformerDecoderOptions`-struct constructor forms,
given the same `seed`, produce bit-for-bit identical output -- shown here
via `operator()` on one side and `forward` on the other, since both
resolve to the same call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoder via_positional(4, 2, /*num_layers=*/2, /*dim_feedforward=*/8,
                                        /*dropout=*/0.0, nn::TransformerActivation::ReLU,
                                        /*batch_first=*/false, /*norm_first=*/false,
                                        /*bias=*/true, pluss::Config{}, /*seed=*/3);

  nn::TransformerDecoderOptions opts{};
  opts.d_model = 4;
  opts.nhead = 2;
  opts.num_layers = 2;
  opts.dim_feedforward = 8;
  opts.dropout = 0.0;
  opts.seed = 3;
  nn::TransformerDecoder via_options(opts);

  const pluss::Series tgt = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}, {2, 1, 4});
  const pluss::Series memory = pluss::make_series({0.5, 0.5, 0.0, 0.0, 0.0, 0.5, 0.5, 0.0}, {2, 1, 4});

  const pluss::Series out_positional = via_positional(tgt, memory);  // operator()
  const pluss::Series out_options = via_options.forward(tgt, memory);

  std::cout << "same seed (3), positional and options-struct forms produce identical output: "
            << std::boolalpha << out_positional.allclose(out_options) << "\n";

  return 0;
}
```

Output

```text
same seed (3), positional and options-struct forms produce identical output: true
```

### Example 4

`reset_parameters()` with the same `seed` reproduces every internal
layer's exact original weights, including deep inside `layer(0)`'s own
`linear1`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoder decoder(4, 2, /*num_layers=*/2, /*dim_feedforward=*/8, /*dropout=*/0.0,
                                 nn::TransformerActivation::ReLU, false, false, true, pluss::Config{}, /*seed=*/9);

  const pluss::Series layer0_weight_before = decoder->layer(0)->linear1()->weight().clone();

  decoder->reset_parameters();

  std::cout << "reset_parameters() with the same seed reproduces layer(0)'s linear1 weight: "
            << std::boolalpha << layer0_weight_before.allclose(decoder->layer(0)->linear1()->weight()) << "\n";

  return 0;
}
```

Output

```text
reset_parameters() with the same seed reproduces layer(0)'s linear1 weight: true
```

### Example 5

`TransformerDecoder` runs on the MPS backend the same way it does on
CPU -- moving the stack with `.to(pMPS)` moves every internal layer's
parameters, including each layer's `cross_attn`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::TransformerDecoder decoder(4, 2, /*num_layers=*/2, /*dim_feedforward=*/8);
  decoder.to(pluss::pMPS);

  std::cout << "layer(0) cross_attn q_weight platform after .to(pMPS): "
            << static_cast<int>(decoder->layer(0)->cross_attn()->q_weight().platform()) << "\n";

  const Series tgt = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  const Series memory = pluss::make_series({0.2, -0.1, 0.4, 0.0}, {1, 1, 4}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series out = decoder.forward(tgt, memory);
    std::cout << "transformer decoder on MPS: OK, output platform=" << static_cast<int>(out.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "transformer decoder on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
layer(0) cross_attn q_weight platform after .to(pMPS): 2
transformer decoder on MPS: OK, output platform=2
```

## Changelog

- **0.1** — a stack of `TransformerDecoderLayer` blocks, `num_layers`/
  `final_norm` on top of every per-layer option; `depth()`/`layer(index)`
  accessors, the positional constructor, and `operator()`.

## See Also

- [`TransformerDecoderOptions`](TransformerDecoderOptions.md) -- the
  options struct.
- [`TransformerDecoderLayer`](TransformerDecoderLayer.md) -- the
  repeating unit this stacks.
- [`TransformerEncoder`](TransformerEncoder.md) -- the analogous
  encoder-side stack, whose output is the usual source of this class's
  `memory` argument (see [`Transformer`](Transformer.md)).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TransformerDecoderOptions
```
<!-- apiref:toctree:end -->
