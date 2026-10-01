# `pluss::ai::nn::Transformer`

The full encoder-decoder transformer: an internal
[`TransformerEncoder`](TransformerEncoder.md) and an internal
[`TransformerDecoder`](TransformerDecoder.md), composed the standard way
-- `encoder()` runs over `src` to produce `memory`, then `decoder()` runs
over `tgt`, cross-attending to that `memory`. The two internal stacks
share every option in [`TransformerOptions`](TransformerOptions.md)
except their own independent layer counts (`num_encoder_layers`/
`num_decoder_layers`) and independently-offset random seeds.

`forward` has two overloads: the two-argument encoder-decoder form
(`forward(src, tgt)`, the standard sequence-to-sequence path), and a
one-argument encoder-only form (`forward(src)`, equivalent to calling
[`encode(src)`](#methods) directly) for when only the encoder side is
needed -- see Example 2.

## Signature

```cpp
explicit nn::Transformer::Transformer(nn::TransformerOptions options);
nn::Transformer::Transformer(
    std::int64_t d_model, std::int64_t nhead, std::int64_t num_encoder_layers = 6,
    std::int64_t num_decoder_layers = 6, std::int64_t dim_feedforward = 2048,
    double dropout = 0.1, nn::TransformerActivation activation = nn::TransformerActivation::ReLU,
    bool batch_first = false, bool norm_first = false, bool bias = true,
    pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## Construction

Two equivalent forms: pass a
[`TransformerOptions`](TransformerOptions.md) directly, or spell out the
most-used fields as positional arguments:

```cpp
nn::Transformer a(nn::TransformerOptions{512, 8});  // d_model=512, nhead=8, 6 encoder + 6 decoder layers (defaults)
nn::Transformer b(512, 8);                            // equivalent positional form
```

Unlike every other class in this family, `Transformer` has no
`reset_parameters()` of its own -- reinitializing means calling
[`encoder()`](#state)`->reset_parameters()` and
[`decoder()`](#state)`->reset_parameters()` separately, since this class
owns no parameters directly, only the two sub-stacks that do.

## State

`encoder()` (mutable and `const` overloads) -- `TransformerEncoder&` /
`const TransformerEncoder&`
: The internal encoder stack, built with `num_encoder_layers` layers.
  Reachable to inspect any individual layer, or to run
  [`encode(src)`](#methods) explicitly.

`decoder()` (mutable and `const` overloads) -- `TransformerDecoder&` /
`const TransformerDecoder&`
: The internal decoder stack, built with `num_decoder_layers` layers.

## Methods

`forward(src, tgt, src_mask = {}, tgt_mask = {}, memory_mask = {}, src_key_padding_mask = {}, tgt_key_padding_mask = {}, memory_key_padding_mask = {}, src_is_causal = false, tgt_is_causal = false, memory_is_causal = false)` --
`Series`
: Runs `encoder()->forward(src, src_mask, src_key_padding_mask,
  src_is_causal)` to produce `memory`, then returns
  `decoder()->forward(tgt, memory, tgt_mask, memory_mask,
  tgt_key_padding_mask, memory_key_padding_mask, tgt_is_causal,
  memory_is_causal)`. Same shape as `tgt`. `src`'s and `tgt`'s last axis
  must both equal `options().d_model`; when both are batched (rank 3),
  their batch axes must also match.

`forward(src)` -- `Series`
: The encoder-only shortcut -- exactly `encoder()->forward(src)`, with no
  masks. Equivalent to calling [`encode(src)`](#methods) directly. Useful
  when only an encoder representation is needed, without running the
  decoder side at all.

`encode(src, src_mask = {}, src_key_padding_mask = {}, src_is_causal = false)`
(reachable only via `->`) -- `Series`
: The full-control encoder-only entry point -- identical to
  `encoder()->forward(...)` with the same arguments, spelled out at the
  `Transformer` level for convenience. `forward(src)` is this call with
  every mask/causal argument left at its default.

`operator()(src, tgt)` / `operator()(src)` -- `Series`
: Convenience overloads calling `forward(src, tgt)` / `forward(src)`
  respectively. Unlike `forward` itself, `operator()` has no overload
  taking the full mask/causal argument list -- reach for `forward` (or
  `encode`) directly when any of those are needed.

## Practical Examples

Five examples: the full encoder-decoder path end to end, the
encoder-only `forward(src)`/`encode(src)`/`operator()(src)` forms shown
equivalent, independent encoder/decoder depths accessed through
`encoder()`/`decoder()`, the positional-vs-options-struct construction
equivalence, and confirming the whole model runs on the MPS backend.

### Example 1

A small `d_model=4` model with 2 encoder layers and 2 decoder layers,
run end to end: `encoder()` transforms `src` into `memory`, `decoder()`
then transforms `tgt` while cross-attending to that `memory`. The output
takes `tgt`'s shape, regardless of `src`'s own sequence length.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Transformer transformer(4, 2, /*num_encoder_layers=*/2, /*num_decoder_layers=*/2,
                              /*dim_feedforward=*/8, /*dropout=*/0.0);

  // batch_first=false (default): src=[seq_len=3,batch=1,d_model=4], tgt=[seq_len=2,batch=1,d_model=4]
  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 1, 4});
  const pluss::Series tgt = pluss::make_series(
      {0.5, 0.5, 0.0, 0.0,
       0.0, 0.5, 0.5, 0.0},
      {2, 1, 4});

  const pluss::Series out = transformer.forward(src, tgt);

  std::cout << "src shape: [" << src.sizes()[0] << ", " << src.sizes()[1] << ", " << src.sizes()[2] << "]\n";
  std::cout << "tgt shape: [" << tgt.sizes()[0] << ", " << tgt.sizes()[1] << ", " << tgt.sizes()[2] << "]\n";
  std::cout << "output shape (same as tgt): [" << out.sizes()[0] << ", " << out.sizes()[1] << ", " << out.sizes()[2] << "]\n";

  return 0;
}
```

Output

```text
src shape: [3, 1, 4]
tgt shape: [2, 1, 4]
output shape (same as tgt): [2, 1, 4]
```

### Example 2

`forward(src)` (encoder-only), `encode(src)`, and `operator()(src)` all
compute the exact same thing -- three names for the same encoder-only
path.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Transformer transformer(4, 2, /*num_encoder_layers=*/2, /*num_decoder_layers=*/2, /*dim_feedforward=*/8);

  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 1, 4});

  // encoder-only single-arg forward
  const pluss::Series out_forward = transformer.forward(src);
  // explicit encode() -- same computation, different name
  const pluss::Series out_encode = transformer->encode(src);
  // operator()(src) -- the single-arg convenience form
  const pluss::Series out_call = transformer(src);

  std::cout << "forward(src) == encode(src): " << std::boolalpha << out_forward.allclose(out_encode) << "\n";
  std::cout << "forward(src) == operator()(src): " << out_forward.allclose(out_call) << "\n";
  std::cout << "output shape: [" << out_forward.sizes()[0] << ", " << out_forward.sizes()[1] << ", " << out_forward.sizes()[2] << "]\n";

  return 0;
}
```

Output

```text
forward(src) == encode(src): true
forward(src) == operator()(src): true
output shape: [2, 1, 4]
```

### Example 3

`num_encoder_layers` and `num_decoder_layers` are fully independent --
here a 4-layer encoder feeds a 1-layer decoder. `encoder()`/`decoder()`
each report their own `depth()` directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // asymmetric depth: a deeper encoder, a shallower decoder
  nn::Transformer transformer(4, 2, /*num_encoder_layers=*/4, /*num_decoder_layers=*/1, /*dim_feedforward=*/8);

  std::cout << "encoder()->depth()=" << transformer->encoder()->depth() << "\n";
  std::cout << "decoder()->depth()=" << transformer->decoder()->depth() << "\n";

  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 1, 4});
  const pluss::Series tgt = pluss::make_series({0.5, 0.5, 0.0, 0.0}, {1, 1, 4});

  const pluss::Series out = transformer.forward(src, tgt);
  std::cout << "output shape: [" << out.sizes()[0] << ", " << out.sizes()[1] << ", " << out.sizes()[2] << "]\n";

  return 0;
}
```

Output

```text
encoder()->depth()=4
decoder()->depth()=1
output shape: [1, 1, 4]
```

### Example 4

The positional and `TransformerOptions`-struct constructor forms, given
the same `seed`, produce bit-for-bit identical output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::Transformer via_positional(4, 2, /*num_encoder_layers=*/2, /*num_decoder_layers=*/2,
                                 /*dim_feedforward=*/8, /*dropout=*/0.0, nn::TransformerActivation::ReLU,
                                 /*batch_first=*/false, /*norm_first=*/false, /*bias=*/true,
                                 pluss::Config{}, /*seed=*/11);

  nn::TransformerOptions opts{};
  opts.d_model = 4;
  opts.nhead = 2;
  opts.num_encoder_layers = 2;
  opts.num_decoder_layers = 2;
  opts.dim_feedforward = 8;
  opts.dropout = 0.0;
  opts.seed = 11;
  nn::Transformer via_options(opts);

  const pluss::Series src = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}, {2, 1, 4});
  const pluss::Series tgt = pluss::make_series({0.5, 0.5, 0.0, 0.0}, {1, 1, 4});

  const pluss::Series out_positional = via_positional.forward(src, tgt);
  const pluss::Series out_options = via_options.forward(src, tgt);

  std::cout << "same seed (11), positional and options-struct forms produce identical output: "
            << std::boolalpha << out_positional.allclose(out_options) << "\n";

  return 0;
}
```

Output

```text
same seed (11), positional and options-struct forms produce identical output: true
```

### Example 5

`Transformer` runs on the MPS backend the same way it does on CPU --
moving the model with `.to(pMPS)` moves every parameter in both the
internal encoder and decoder.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::Transformer transformer(4, 2, /*num_encoder_layers=*/2, /*num_decoder_layers=*/2, /*dim_feedforward=*/8);
  transformer.to(pluss::pMPS);

  std::cout << "encoder()->layer(0) self_attn q_weight platform after .to(pMPS): "
            << static_cast<int>(transformer->encoder()->layer(0)->self_attn()->q_weight().platform()) << "\n";

  const Series src = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  const Series tgt = pluss::make_series({0.2, -0.1, 0.4, 0.0}, {1, 1, 4}).to(pluss::ScalarType::Float32).to(pluss::pMPS);
  try {
    const Series out = transformer.forward(src, tgt);
    std::cout << "transformer on MPS: OK, output platform=" << static_cast<int>(out.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "transformer on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
encoder()->layer(0) self_attn q_weight platform after .to(pMPS): 2
transformer on MPS: OK, output platform=2
```

## Changelog

- **0.1** — the full encoder-decoder transformer, composed from
  `TransformerEncoder`/`TransformerDecoder`; `d_model`/`nhead`/
  `num_encoder_layers`/`num_decoder_layers`/`dim_feedforward`/`dropout`/
  `activation`/`layer_norm_eps`/`batch_first`/`norm_first`/`bias`
  options; `encoder()`/`decoder()`/`options()` accessors.

## See Also

- [`TransformerOptions`](TransformerOptions.md) -- the options struct.
- [`TransformerEncoder`](TransformerEncoder.md) /
  [`TransformerDecoder`](TransformerDecoder.md) -- the two internal
  stacks this composes.
- [`Introduction`](Introduction.md) -- mentions
  `generate_square_subsequent_mask`/`generate_causal_mask`, the two
  free functions in this family for building the `tgt_mask` an
  autoregressive decoder typically needs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TransformerOptions
```
<!-- apiref:toctree:end -->
