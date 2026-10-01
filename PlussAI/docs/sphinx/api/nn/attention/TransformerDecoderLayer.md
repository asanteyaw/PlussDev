# `pluss::ai::nn::TransformerDecoderLayer`

One self-attention + cross-attention + feedforward decoder block: a
[`MultiheadAttention`](MultiheadAttention.md) sub-module (`self_attn`)
runs self-attention over `tgt` (`query == key == value == tgt`), a
second, independent `MultiheadAttention` sub-module (`cross_attn`) then
runs cross-attention with `tgt` as the query and `memory` (typically an
encoder stack's output) as both key and value, and finally a two-layer
feedforward sub-network runs as in
[`TransformerEncoderLayer`](TransformerEncoderLayer.md). A residual
connection and a layer normalization wrap each of the three sub-blocks --
one more sub-block, and one more normalization, than the encoder-side
layer. This is the repeating unit
[`TransformerDecoder`](TransformerDecoder.md) stacks `num_layers` times.

`norm_first` changes the actual arithmetic of all three sub-blocks, not
just where a bookkeeping step sits:

- **post-norm** (`norm_first = false`, the default): `x = norm1(x +
  self_attn(x)); x = norm2(x + cross_attn(x, memory)); x = norm3(x +
  ff(x))`.
- **pre-norm** (`norm_first = true`): `x = x + self_attn(norm1(x)); x = x
  + cross_attn(norm2(x), memory); x = x + ff(norm3(x))`.

See Example 2 for both computed from the same weights.

## Signature

```cpp
explicit nn::TransformerDecoderLayer::TransformerDecoderLayer(nn::TransformerDecoderLayerOptions options);
nn::TransformerDecoderLayer::TransformerDecoderLayer(
    std::int64_t d_model, std::int64_t nhead, std::int64_t dim_feedforward = 2048,
    double dropout = 0.1, nn::TransformerActivation activation = nn::TransformerActivation::ReLU,
    bool batch_first = false, bool norm_first = false, bool bias = true,
    pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::TransformerDecoderLayer::reset_parameters();
```

## Construction

Two equivalent forms: pass a
[`TransformerDecoderLayerOptions`](TransformerDecoderLayerOptions.md)
directly, or spell out the most-used fields as positional arguments. The
positional form has no parameters for `layer_norm_eps` or
`average_attn_weights` -- both stay at their defaults (`1e-5`, `true`)
unless the options-struct form is used:

```cpp
nn::TransformerDecoderLayer a(nn::TransformerDecoderLayerOptions{512, 8});  // d_model=512, nhead=8, everything else defaulted
nn::TransformerDecoderLayer b(512, 8);                                      // equivalent positional form
```

## State

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the
[`TransformerDecoderLayerOptions`](TransformerDecoderLayerOptions.md)
this instance was constructed with.

`self_attn()` / `cross_attn()` (mutable and `const` overloads) --
`MultiheadAttention&` / `const MultiheadAttention&`
: The two internal attention sub-modules. `self_attn` attends over `tgt`
  alone; `cross_attn` uses `tgt` as its query and `memory` as both key
  and value. Both are built from a
  [`MultiheadAttentionOptions`](MultiheadAttentionOptions.md) derived
  from this block's own `d_model`/`nhead`/`dropout`/`bias`/`batch_first`/
  `average_attn_weights`.

`linear1()` / `linear2()` (mutable and `const` overloads) -- `Fusion&` /
`const Fusion&`
: The two [`Fusion`](../fusion/Fusion.md) layers making up the
  feedforward sub-network, identical in role to
  [`TransformerEncoderLayer::linear1()`/`linear2()`](TransformerEncoderLayer.md#state).

## Methods

`forward(tgt, memory, tgt_mask = {}, memory_mask = {}, tgt_key_padding_mask = {}, memory_key_padding_mask = {}, tgt_is_causal = false, memory_is_causal = false)` /
`operator()(...)` -- `Series`
: Runs one decoder block over `tgt` (attending to itself, then to
  `memory`) and returns the transformed result, same shape as `tgt`.
  `tgt_mask`/`tgt_key_padding_mask`/`tgt_is_causal` are forwarded to the
  internal `self_attn`'s own
  [`forward`](MultiheadAttention.md#methods); `memory_mask`/
  `memory_key_padding_mask`/`memory_is_causal` are forwarded to
  `cross_attn`'s own `forward` (as its `attn_mask`/`key_padding_mask`/
  `causal`, with `tgt` as `query` and `memory` as both `key` and
  `value`). `tgt_is_causal` is the one most autoregressive decoding
  actually needs -- it prevents `tgt`'s own self-attention from looking
  ahead at later target positions, the same mechanism
  [`MultiheadAttention`](MultiheadAttention.md)'s own `causal=true`
  provides.

`reset_parameters()` -- `void`
: Re-delegates to `self_attn()`'s, `cross_attn()`'s, `linear1()`'s, and
  `linear2()`'s own `reset_parameters()` (each in place), then re-fills
  this block's own three layer-normalization weight/bias pairs in place
  (`ones`/`zeros`). Called once during construction; call it again to
  reinitialize an already-trained instance.

## Practical Examples

Five examples: a shape/layer-norm-invariant check on a small block with a
`memory` of a different sequence length than `tgt`, the same weights run
through post-norm and pre-norm showing the outputs genuinely differ,
`activation=GELU` alongside the positional-vs-options-struct construction
equivalence, inspecting all four internal sub-modules directly, and
confirming the block runs on the MPS backend.

### Example 1

`tgt` has 2 timesteps, `memory` has 3 -- cross-attention handles a
`memory` of a different length than `tgt` without any special handling,
since `cross_attn`'s query comes from `tgt` and its key/value come from
`memory` independently. Post-norm (the default) still leaves every output
row with mean approximately `0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoderLayer layer(4, 2, /*dim_feedforward=*/8, /*dropout=*/0.0);

  // batch_first=false (default): tgt=[seq_len=2,batch=1,d_model=4], memory=[seq_len=3,batch=1,d_model=4]
  const pluss::Series tgt = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 1, 4});
  const pluss::Series memory = pluss::make_series(
      {0.5, 0.5, 0.0, 0.0,
       0.0, 0.5, 0.5, 0.0,
       0.0, 0.0, 0.5, 0.5},
      {3, 1, 4});

  const pluss::Series out = layer.forward(tgt, memory);

  std::cout << "tgt shape:    [" << tgt.sizes()[0] << ", " << tgt.sizes()[1] << ", " << tgt.sizes()[2] << "]\n";
  std::cout << "memory shape: [" << memory.sizes()[0] << ", " << memory.sizes()[1] << ", " << memory.sizes()[2] << "]\n";
  std::cout << "output shape: [" << out.sizes()[0] << ", " << out.sizes()[1] << ", " << out.sizes()[2] << "]\n";

  const pluss::Series flat = out.reshape({2, 4});
  std::cout << "row means (post-norm output, should be ~0): " << flat.mean(-1, false) << "\n";

  return 0;
}
```

Output

```text
tgt shape:    [2, 1, 4]
memory shape: [3, 1, 4]
output shape: [2, 1, 4]
row means (post-norm output, should be ~0): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[8.3266727e-17, 2.7755576e-17]
```

### Example 2

The exact same weights (same `seed`), run once with `norm_first=false`
and once with `norm_first=true` -- the outputs genuinely differ.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoderLayerOptions post_opts{};
  post_opts.d_model = 4;
  post_opts.nhead = 2;
  post_opts.dim_feedforward = 8;
  post_opts.dropout = 0.0;
  post_opts.norm_first = false;
  post_opts.seed = 5;
  nn::TransformerDecoderLayer post_norm(post_opts);

  nn::TransformerDecoderLayerOptions pre_opts = post_opts;
  pre_opts.norm_first = true;
  nn::TransformerDecoderLayer pre_norm(pre_opts);

  const pluss::Series tgt = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 1, 4});
  const pluss::Series memory = pluss::make_series(
      {0.5, 0.5, 0.0, 0.0,
       0.0, 0.5, 0.5, 0.0},
      {2, 1, 4});

  const pluss::Series post_out = post_norm.forward(tgt, memory);
  const pluss::Series pre_out = pre_norm.forward(tgt, memory);

  std::cout << "post-norm output row 0: " << post_out.reshape({2, 4}).slice(0, 0, 1) << "\n";
  std::cout << "pre-norm  output row 0: " << pre_out.reshape({2, 4}).slice(0, 0, 1) << "\n";
  std::cout << "identical weights, different placement -- outputs differ: "
            << std::boolalpha << !post_out.allclose(pre_out) << "\n";

  return 0;
}
```

Output

```text
post-norm output row 0: pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.5257679, -1.1905994, 0.14728923, -0.48245774]]
pre-norm  output row 0: pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.2922779, 0.53745298, 0.75012967, 0.57963017]]
identical weights, different placement -- outputs differ: true
```

### Example 3

`activation=GELU`, alongside the `TransformerDecoderLayerOptions`-struct
and positional constructor forms producing identical options and,
with the same default `seed`, identical output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoderLayer via_positional(4, 2, /*dim_feedforward=*/8, /*dropout=*/0.0,
                                             nn::TransformerActivation::GELU);

  nn::TransformerDecoderLayerOptions opts{};
  opts.d_model = 4;
  opts.nhead = 2;
  opts.dim_feedforward = 8;
  opts.dropout = 0.0;
  opts.activation = nn::TransformerActivation::GELU;
  nn::TransformerDecoderLayer via_options(opts);

  std::cout << "via_positional activation is GELU=" << std::boolalpha
            << (via_positional->options().activation == nn::TransformerActivation::GELU) << "\n";
  std::cout << "via_options    activation is GELU=" << (via_options->options().activation == nn::TransformerActivation::GELU) << "\n";

  via_options->reset_parameters();
  via_positional->reset_parameters();

  const pluss::Series tgt = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4});
  const pluss::Series memory = pluss::make_series({0.2, -0.1, 0.4, 0.0}, {1, 1, 4});

  std::cout << "same seed (default 0), same output: "
            << via_positional.forward(tgt, memory).allclose(via_options.forward(tgt, memory)) << "\n";

  return 0;
}
```

Output

```text
via_positional activation is GELU=true
via_options    activation is GELU=true
same seed (default 0), same output: true
```

### Example 4

`self_attn()`/`cross_attn()`/`linear1()`/`linear2()` expose all four
internal sub-modules directly. `parameters()` (inherited from `Module`)
sees every parameter across all four plus this block's own three
layer-normalization pairs -- one more pair than
[`TransformerEncoderLayer`](TransformerEncoderLayer.md#practical-examples)'s
two, reflecting the third (cross-attention) sub-block.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerDecoderLayer layer(4, 2, /*dim_feedforward=*/8);

  std::cout << "self_attn embed_dim=" << layer->self_attn()->options().embed_dim << "\n";
  std::cout << "cross_attn embed_dim=" << layer->cross_attn()->options().embed_dim << "\n";
  std::cout << "linear1: in_features=" << layer->linear1()->options().in_features
            << ", out_features=" << layer->linear1()->options().out_features << "\n";
  std::cout << "linear2: in_features=" << layer->linear2()->options().in_features
            << ", out_features=" << layer->linear2()->options().out_features << "\n";

  std::cout << "layer.parameters().size()=" << layer.parameters().size()
            << " -- self_attn(8) + cross_attn(8) + linear1(2) + linear2(2) + norm1/2/3(2 each=6) = 26\n";

  return 0;
}
```

Output

```text
self_attn embed_dim=4
cross_attn embed_dim=4
linear1: in_features=4, out_features=8
linear2: in_features=8, out_features=4
layer.parameters().size()=26 -- self_attn(8) + cross_attn(8) + linear1(2) + linear2(2) + norm1/2/3(2 each=6) = 26
```

### Example 5

`TransformerDecoderLayer` runs on the MPS backend the same way it does on
CPU -- moving the block with `.to(pMPS)` moves every internal
sub-module's parameters, including `cross_attn`'s.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::TransformerDecoderLayer layer(4, 2, /*dim_feedforward=*/8);
  layer.to(pluss::pMPS);

  std::cout << "cross_attn q_weight platform after .to(pMPS): "
            << static_cast<int>(layer->cross_attn()->q_weight().platform()) << "\n";

  const Series tgt = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  const Series memory = pluss::make_series({0.2, -0.1, 0.4, 0.0}, {1, 1, 4}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series out = layer.forward(tgt, memory);
    std::cout << "transformer decoder layer on MPS: OK, output platform=" << static_cast<int>(out.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "transformer decoder layer on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
cross_attn q_weight platform after .to(pMPS): 2
transformer decoder layer on MPS: OK, output platform=2
```

## Changelog

- **0.1** — one self-attention + cross-attention + feedforward decoder
  block, `d_model`/`nhead`/`dim_feedforward`/`dropout`/`activation`/
  `layer_norm_eps`/`batch_first`/`norm_first`/`bias`/
  `average_attn_weights` options; `self_attn()`/`cross_attn()`/
  `linear1()`/`linear2()`/`options()` accessors.

## See Also

- [`TransformerDecoderLayerOptions`](TransformerDecoderLayerOptions.md)
  -- the options struct.
- [`TransformerDecoder`](TransformerDecoder.md) -- stacks `num_layers` of
  these blocks.
- [`MultiheadAttention`](MultiheadAttention.md) -- both internal
  `self_attn` and `cross_attn` sub-modules.
- [`TransformerEncoderLayer`](TransformerEncoderLayer.md) -- the
  analogous encoder-side block, without cross-attention.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TransformerDecoderLayerOptions
```
<!-- apiref:toctree:end -->
