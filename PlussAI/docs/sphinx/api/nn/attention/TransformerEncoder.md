# `pluss::ai::nn::TransformerEncoder`

A stack of [`TransformerEncoderLayer`](TransformerEncoderLayer.md)
blocks, `num_layers` deep, sharing every option except a per-layer random
seed offset (so no two layers start with identical weights), with an
optional final layer normalization applied once after every layer has
run. `forward` feeds `src` through layer 0, layer 0's output into layer
1, and so on; each layer's own `src_mask`/`src_key_padding_mask`/
`is_causal` are the same for every layer, since `forward` here takes them
once and forwards them to each layer identically.

## Signature

```cpp
explicit nn::TransformerEncoder::TransformerEncoder(nn::TransformerEncoderOptions options);
nn::TransformerEncoder::TransformerEncoder(
    std::int64_t d_model, std::int64_t nhead, std::int64_t num_layers = 6,
    std::int64_t dim_feedforward = 2048, double dropout = 0.1,
    nn::TransformerActivation activation = nn::TransformerActivation::ReLU,
    bool batch_first = false, bool norm_first = false, bool bias = true,
    pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::TransformerEncoder::reset_parameters();
```

## Construction

Two equivalent forms: pass a
[`TransformerEncoderOptions`](TransformerEncoderOptions.md) directly, or
spell out the most-used fields as positional arguments. The positional
form has no parameters for `layer_norm_eps` or `final_norm` -- both stay
at their defaults (`1e-5`, `true`) unless the options-struct form is
used:

```cpp
nn::TransformerEncoder a(nn::TransformerEncoderOptions{512, 8});  // d_model=512, nhead=8, num_layers=6 (default), everything else defaulted
nn::TransformerEncoder b(512, 8);                                  // equivalent positional form
```

## State

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the [`TransformerEncoderOptions`](TransformerEncoderOptions.md)
this instance was constructed with.

`depth()` (`const`) -- `std::size_t`
: The number of internal `TransformerEncoderLayer` instances -- always
  exactly `options().num_layers`.

`layer(index)` (mutable and `const` overloads) -- `TransformerEncoderLayer&` /
`const TransformerEncoderLayer&`
: The internal layer at `index` (`0 <= index < depth()`), in the order
  `forward` runs them. Throws if `index >= depth()`. Reachable to inspect
  or set any specific layer's weights directly, or to reach that layer's
  own `self_attn()`/`linear1()`/`linear2()` sub-modules.

## Methods

`forward(src, mask = {}, src_key_padding_mask = {}, is_causal = false)` /
`operator()(...)` -- `Series`
: Runs `src` through every internal layer in order, then through the
  final layer normalization when `options().final_norm == true`. Same
  shape as `src`. `mask`/`src_key_padding_mask`/`is_causal` are forwarded
  identically to every layer's own
  [`forward`](TransformerEncoderLayer.md#methods).

`reset_parameters()` -- `void`
: Re-delegates to every internal layer's own `reset_parameters()` (each
  in place), then, when `options().final_norm == true`, re-fills this
  class's own final normalization weight/bias pair in place (`ones`/
  `zeros`, the standard layer-norm identity initialization). Called once
  during construction; call it again to reinitialize an already-trained
  stack.

## Practical Examples

Five examples: a 3-layer stack's shape and per-layer weight
distinctness, `final_norm=true` versus `false` and the parameter-count
difference it produces, the positional-vs-options-struct construction
equivalence, confirming `reset_parameters()`'s reproducibility, and
confirming the stack runs on the MPS backend.

### Example 1

A 3-layer stack preserves `src`'s shape; `depth()` reports `3`, and
`layer(0)`/`layer(1)` -- despite sharing every option except their
per-layer seed offset -- do not start with identical weights.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerEncoder encoder(4, 2, /*num_layers=*/3, /*dim_feedforward=*/8, /*dropout=*/0.0);

  std::cout << "depth()=" << encoder->depth() << "\n";

  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 1, 4});

  const pluss::Series out = encoder.forward(src);
  std::cout << "output shape: [" << out.sizes()[0] << ", " << out.sizes()[1] << ", " << out.sizes()[2] << "]\n";

  // each layer derives its own seed offset -- confirm layer 0 and layer 1
  // do not start with identical weights despite sharing every other option
  const bool same_weight = encoder->layer(0)->linear1()->weight().allclose(encoder->layer(1)->linear1()->weight());
  std::cout << "layer(0) and layer(1) linear1 weights identical: " << std::boolalpha << same_weight << "\n";

  return 0;
}
```

Output

```text
depth()=3
output shape: [3, 1, 4]
layer(0) and layer(1) linear1 weights identical: false
```

### Example 2

`final_norm=false` registers no final normalization parameters at all --
exactly 2 fewer entries in `parameters()` than the `true` (default) case,
and the output is exactly the last layer's own raw output, no longer
constrained to have a ~0 row mean.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerEncoderOptions with_norm{};
  with_norm.d_model = 4;
  with_norm.nhead = 2;
  with_norm.num_layers = 2;
  with_norm.dim_feedforward = 8;
  with_norm.dropout = 0.0;
  with_norm.final_norm = true;
  nn::TransformerEncoder encoder_with_norm(with_norm);

  nn::TransformerEncoderOptions without_norm = with_norm;
  without_norm.final_norm = false;
  nn::TransformerEncoder encoder_without_norm(without_norm);

  std::cout << "with final_norm: parameters().size()=" << encoder_with_norm.parameters().size() << "\n";
  std::cout << "without final_norm: parameters().size()=" << encoder_without_norm.parameters().size()
            << " -- exactly 2 fewer (no norm_weight/norm_bias)\n";

  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 1, 4});

  const pluss::Series out_with = encoder_with_norm.forward(src);
  std::cout << "with final_norm, output row means (should be ~0): "
            << out_with.reshape({2, 4}).mean(-1, false) << "\n";

  return 0;
}
```

Output

```text
with final_norm: parameters().size()=34
without final_norm: parameters().size()=32 -- exactly 2 fewer (no norm_weight/norm_bias)
with final_norm, output row means (should be ~0): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2.7755576e-17, 2.7755576e-17]
```

### Example 3

The positional and `TransformerEncoderOptions`-struct constructor forms,
given the same `seed`, produce bit-for-bit identical output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerEncoder via_positional(4, 2, /*num_layers=*/2, /*dim_feedforward=*/8,
                                        /*dropout=*/0.0, nn::TransformerActivation::ReLU,
                                        /*batch_first=*/false, /*norm_first=*/false,
                                        /*bias=*/true, pluss::Config{}, /*seed=*/3);

  nn::TransformerEncoderOptions opts{};
  opts.d_model = 4;
  opts.nhead = 2;
  opts.num_layers = 2;
  opts.dim_feedforward = 8;
  opts.dropout = 0.0;
  opts.seed = 3;
  nn::TransformerEncoder via_options(opts);

  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 1, 4});

  const pluss::Series out_positional = via_positional.forward(src);
  const pluss::Series out_options = via_options.forward(src);

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

  nn::TransformerEncoder encoder(4, 2, /*num_layers=*/2, /*dim_feedforward=*/8, /*dropout=*/0.0,
                                 nn::TransformerActivation::ReLU, false, false, true, pluss::Config{}, /*seed=*/9);

  const pluss::Series layer0_weight_before = encoder->layer(0)->linear1()->weight().clone();

  encoder->reset_parameters();  // same seed -- reproduces the exact same weights

  std::cout << "reset_parameters() with the same seed reproduces layer(0)'s linear1 weight: "
            << std::boolalpha << layer0_weight_before.allclose(encoder->layer(0)->linear1()->weight()) << "\n";

  return 0;
}
```

Output

```text
reset_parameters() with the same seed reproduces layer(0)'s linear1 weight: true
```

### Example 5

`TransformerEncoder` runs on the MPS backend the same way it does on
CPU -- moving the stack with `.to(pMPS)` moves every internal layer's
parameters.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::TransformerEncoder encoder(4, 2, /*num_layers=*/2, /*dim_feedforward=*/8);
  encoder.to(pluss::pMPS);

  std::cout << "layer(0) self_attn q_weight platform after .to(pMPS): "
            << static_cast<int>(encoder->layer(0)->self_attn()->q_weight().platform()) << "\n";

  const Series src = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4})
                          .to(pluss::ScalarType::Float32)
                          .to(pluss::pMPS);
  try {
    const Series out = encoder.forward(src);
    std::cout << "transformer encoder on MPS: OK, output platform=" << static_cast<int>(out.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "transformer encoder on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
layer(0) self_attn q_weight platform after .to(pMPS): 2
transformer encoder on MPS: OK, output platform=2
```

## Changelog

- **0.1** — a stack of `TransformerEncoderLayer` blocks, `num_layers`/
  `final_norm` on top of every per-layer option; `depth()`/`layer(index)`
  accessors.

## See Also

- [`TransformerEncoderOptions`](TransformerEncoderOptions.md) -- the
  options struct.
- [`TransformerEncoderLayer`](TransformerEncoderLayer.md) -- the
  repeating unit this stacks.
- [`TransformerDecoder`](TransformerDecoder.md) -- the analogous
  decoder-side stack, whose `forward` also takes this class's output as
  its `memory` argument (see [`Transformer`](Transformer.md)).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TransformerEncoderOptions
```
<!-- apiref:toctree:end -->
