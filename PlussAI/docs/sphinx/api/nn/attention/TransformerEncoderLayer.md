# `pluss::ai::nn::TransformerEncoderLayer`

One self-attention + feedforward encoder block: a
[`MultiheadAttention`](MultiheadAttention.md) sub-module runs
self-attention over the input (`query == key == value == src`), followed
by a two-layer feedforward sub-network (`linear1` -> activation ->
`linear2`), with a residual connection and a layer normalization around
each of the two sub-blocks. This is the repeating unit
[`TransformerEncoder`](TransformerEncoder.md) stacks `num_layers` times.

Whether normalization runs before or after each sub-block
(`norm_first`) changes the actual arithmetic, not just where a
bookkeeping step sits:

- **post-norm** (`norm_first = false`, the default): `x = norm1(x +
  self_attn(x)); x = norm2(x + ff(x))`.
- **pre-norm** (`norm_first = true`): `x = x + self_attn(norm1(x)); x = x
  + ff(norm2(x))`.

Post-norm is the placement from the original Transformer paper; pre-norm
is more common in later, deeper designs. See Example 2 for both computed
from the same weights.

## Signature

```cpp
explicit nn::TransformerEncoderLayer::TransformerEncoderLayer(nn::TransformerEncoderLayerOptions options);
nn::TransformerEncoderLayer::TransformerEncoderLayer(
    std::int64_t d_model, std::int64_t nhead, std::int64_t dim_feedforward = 2048,
    double dropout = 0.1, nn::TransformerActivation activation = nn::TransformerActivation::ReLU,
    bool batch_first = false, bool norm_first = false, bool bias = true,
    pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::TransformerEncoderLayer::reset_parameters();
```

## Construction

Two equivalent forms: pass a
[`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md)
directly, or spell out the most-used fields as positional arguments. The
positional form has no parameters for `layer_norm_eps` or
`average_attn_weights` -- both stay at their defaults (`1e-5`, `true`)
unless the options-struct form is used:

```cpp
nn::TransformerEncoderLayer a(nn::TransformerEncoderLayerOptions{512, 8});  // d_model=512, nhead=8, everything else defaulted
nn::TransformerEncoderLayer b(512, 8);                                      // equivalent positional form
```

`d_model` must be evenly divisible by `nhead` -- constructing with a
mismatched pair throws immediately.

## State

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the
[`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md)
this instance was constructed with.

`self_attn()` (mutable and `const` overloads) -- `MultiheadAttention&` /
`const MultiheadAttention&`
: The internal self-attention sub-module, built from a
  [`MultiheadAttentionOptions`](MultiheadAttentionOptions.md) derived
  from this block's own `d_model`/`nhead`/`dropout`/`bias`/`batch_first`/
  `average_attn_weights`. Reachable to inspect or set its own weights
  directly, or to read its own attention weights (`forward` on this
  block only returns the transformed `src`, never `self_attn`'s
  weights).

`linear1()` / `linear2()` (mutable and `const` overloads) -- `Fusion&` /
`const Fusion&`
: The two [`Fusion`](../fusion/Fusion.md) layers making up the
  feedforward sub-network: `linear1` maps `d_model -> dim_feedforward`,
  `linear2` maps `dim_feedforward -> d_model`.

## Methods

`forward(src, src_mask = {}, src_key_padding_mask = {}, is_causal = false)` /
`operator()(...)` -- `Series`
: Runs one encoder block over `src` and returns the transformed result,
  same shape as `src`. `src_mask`/`src_key_padding_mask`/`is_causal` are
  forwarded directly to the internal `self_attn`'s own
  [`forward`](MultiheadAttention.md#methods) -- see that page for exactly
  what each blocks. Whether normalization runs before or after each
  sub-block is controlled entirely by `options().norm_first`, not by
  anything passed to `forward` itself.

`reset_parameters()` -- `void`
: Re-delegates to `self_attn()`'s, `linear1()`'s, and `linear2()`'s own
  `reset_parameters()` (each in place), then re-fills this block's own
  two layer-normalization weight/bias pairs in place (`ones`/`zeros`,
  the standard layer-norm identity initialization -- not a random draw).
  Called once during construction; call it again to reinitialize an
  already-trained instance.

## Practical Examples

Five examples: a shape/layer-norm-invariant check on a small block, the
same weights run through post-norm and pre-norm showing the outputs
genuinely differ, `activation=GELU` alongside the positional-vs-options-
struct construction equivalence, inspecting the three internal
sub-modules directly, and confirming the block runs on the MPS backend.

### Example 1

A small `d_model=4`, `nhead=2`, `dim_feedforward=8` block preserves
`src`'s shape, and -- since post-norm (the default) ends every sub-block
with a layer normalization -- every output timestep has mean
approximately `0`, the defining property of a layer-normalized row.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // d_model=4, nhead=2, dim_feedforward=8
  nn::TransformerEncoderLayer layer(4, 2, /*dim_feedforward=*/8, /*dropout=*/0.0);
  layer.eval();  // dropout=0.0 already disables it, but be explicit

  // batch_first=false (default): [seq_len=3, batch=1, d_model=4]
  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 1, 4});

  const pluss::Series out = layer.forward(src);

  std::cout << "input shape:  [" << src.sizes()[0] << ", " << src.sizes()[1] << ", " << src.sizes()[2] << "]\n";
  std::cout << "output shape: [" << out.sizes()[0] << ", " << out.sizes()[1] << ", " << out.sizes()[2] << "]\n";

  // post-norm (norm_first=false, the default): every output timestep is
  // layer-normalized, so each row has mean ~0 and unit variance
  const pluss::Series flat = out.reshape({3, 4});
  std::cout << "row means (post-norm output, should be ~0): " << flat.mean(-1, false) << "\n";

  return 0;
}
```

Output

```text
input shape:  [3, 1, 4]
output shape: [3, 1, 4]
row means (post-norm output, should be ~0): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4.1633363e-17, 1.3877788e-17, 2.7755576e-17]
```

### Example 2

The exact same weights (same `seed`), run once with `norm_first=false`
and once with `norm_first=true` -- the outputs genuinely differ, since
`norm_first` changes where in the arithmetic normalization happens, not
merely a display choice.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerEncoderLayerOptions post_opts{};
  post_opts.d_model = 4;
  post_opts.nhead = 2;
  post_opts.dim_feedforward = 8;
  post_opts.dropout = 0.0;
  post_opts.norm_first = false;
  post_opts.seed = 7;
  nn::TransformerEncoderLayer post_norm(post_opts);

  nn::TransformerEncoderLayerOptions pre_opts = post_opts;
  pre_opts.norm_first = true;
  nn::TransformerEncoderLayer pre_norm(pre_opts);

  const pluss::Series src = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 1, 4});

  const pluss::Series post_out = post_norm.forward(src);
  const pluss::Series pre_out = pre_norm.forward(src);

  std::cout << "post-norm (norm_first=false) output row 0: " << post_out.reshape({3, 4}).slice(0, 0, 1) << "\n";
  std::cout << "pre-norm  (norm_first=true)  output row 0: " << pre_out.reshape({3, 4}).slice(0, 0, 1) << "\n";
  std::cout << "identical weights, different placement -- outputs differ: "
            << std::boolalpha << !post_out.allclose(pre_out) << "\n";

  return 0;
}
```

Output

```text
post-norm (norm_first=false) output row 0: pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.7101112, -0.80572176, -0.5452407, -0.35914876]]
pre-norm  (norm_first=true)  output row 0: pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.55851899, -0.50573158, -0.35795491, -0.25823475]]
identical weights, different placement -- outputs differ: true
```

### Example 3

`activation=GELU` (the positional form's fifth argument), alongside the
`TransformerEncoderLayerOptions`-struct and positional constructor forms
producing identical options and, with the same default `seed`, identical
output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // positional constructor
  nn::TransformerEncoderLayer via_positional(4, 2, /*dim_feedforward=*/8, /*dropout=*/0.0,
                                             nn::TransformerActivation::GELU);

  // options-struct constructor, equivalent fields
  nn::TransformerEncoderLayerOptions opts{};
  opts.d_model = 4;
  opts.nhead = 2;
  opts.dim_feedforward = 8;
  opts.dropout = 0.0;
  opts.activation = nn::TransformerActivation::GELU;
  nn::TransformerEncoderLayer via_options(opts);

  std::cout << "via_positional: d_model=" << via_positional->options().d_model
            << ", activation is GELU=" << std::boolalpha
            << (via_positional->options().activation == nn::TransformerActivation::GELU) << "\n";
  std::cout << "via_options:    d_model=" << via_options->options().d_model
            << ", activation is GELU=" << (via_options->options().activation == nn::TransformerActivation::GELU) << "\n";

  // same seed on both -- reset_parameters() delegates to every internal
  // sub-module's own reset_parameters(), reproducing identical weights
  via_options->reset_parameters();
  via_positional->reset_parameters();
  const pluss::Series src = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4});
  std::cout << "same seed (default 0), same output: "
            << via_positional.forward(src).allclose(via_options.forward(src)) << "\n";

  return 0;
}
```

Output

```text
via_positional: d_model=4, activation is GELU=true
via_options:    d_model=4, activation is GELU=true
same seed (default 0), same output: true
```

### Example 4

`self_attn()`/`linear1()`/`linear2()` expose the three internal
sub-modules directly -- their own options confirm how this block's own
`d_model`/`nhead`/`dim_feedforward` were distributed among them, and
`parameters()` (inherited from `Module`) sees every parameter across all
three plus this block's own two layer-normalization pairs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::TransformerEncoderLayer layer(4, 2, /*dim_feedforward=*/8);

  std::cout << "self_attn embed_dim=" << layer->self_attn()->options().embed_dim
            << ", num_heads=" << layer->self_attn()->options().num_heads << "\n";
  std::cout << "linear1: in_features=" << layer->linear1()->options().in_features
            << ", out_features=" << layer->linear1()->options().out_features << "\n";
  std::cout << "linear2: in_features=" << layer->linear2()->options().in_features
            << ", out_features=" << layer->linear2()->options().out_features << "\n";

  std::cout << "layer.parameters().size()=" << layer.parameters().size()
            << " -- self_attn (4 weights + 4 biases) + linear1 (weight+bias) + linear2 (weight+bias)"
               " + norm1/norm2 (weight+bias each) = 8+2+2+4=16\n";

  return 0;
}
```

Output

```text
self_attn embed_dim=4, num_heads=2
linear1: in_features=4, out_features=8
linear2: in_features=8, out_features=4
layer.parameters().size()=16 -- self_attn (4 weights + 4 biases) + linear1 (weight+bias) + linear2 (weight+bias) + norm1/norm2 (weight+bias each) = 8+2+2+4=16
```

### Example 5

`TransformerEncoderLayer` runs on the MPS backend the same way it does on
CPU -- moving the block with `.to(pMPS)` moves every internal
sub-module's parameters, including `self_attn`'s.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::TransformerEncoderLayer layer(4, 2, /*dim_feedforward=*/8);
  layer.to(pluss::pMPS);

  std::cout << "self_attn q_weight platform after .to(pMPS): "
            << static_cast<int>(layer->self_attn()->q_weight().platform()) << "\n";

  const Series src = pluss::make_series({1.0, 0.5, -0.5, 0.25}, {1, 1, 4})
                          .to(pluss::ScalarType::Float32)
                          .to(pluss::pMPS);
  try {
    const Series out = layer.forward(src);
    std::cout << "transformer encoder layer on MPS: OK, output platform=" << static_cast<int>(out.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "transformer encoder layer on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
self_attn q_weight platform after .to(pMPS): 2
transformer encoder layer on MPS: OK, output platform=2
```

## Changelog

- **0.1** — one self-attention + feedforward encoder block, `d_model`/
  `nhead`/`dim_feedforward`/`dropout`/`activation`/`layer_norm_eps`/
  `batch_first`/`norm_first`/`bias`/`average_attn_weights` options.

## See Also

- [`TransformerEncoderLayerOptions`](TransformerEncoderLayerOptions.md)
  -- the options struct.
- [`TransformerEncoder`](TransformerEncoder.md) -- stacks `num_layers` of
  these blocks.
- [`MultiheadAttention`](MultiheadAttention.md) -- the internal
  `self_attn` sub-module.
- [`TransformerDecoderLayer`](TransformerDecoderLayer.md) -- the
  analogous decoder-side block, with an additional cross-attention
  sub-block.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

TransformerEncoderLayerOptions
```
<!-- apiref:toctree:end -->
