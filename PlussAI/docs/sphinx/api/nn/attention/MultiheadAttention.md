# `pluss::ai::nn::MultiheadAttention`

Learned multi-head scaled dot-product attention over query/key/value
inputs. `embed_dim` is split evenly into `num_heads` independent heads
(each `embed_dim / num_heads` wide); every head runs its own scaled
dot-product attention over its own slice, and the heads' outputs are
concatenated back to `embed_dim` before a final learned output
projection. This is the attention mechanism every layer in this family
that needs attention (`TransformerEncoderLayer`'s `self_attn`,
`TransformerDecoderLayer`'s `self_attn` and `cross_attn`) builds and owns
internally.

Unlike a lower-level scaled-dot-product primitive, `forward` always
computes and returns the attention weights alongside the output -- there
is no toggle to skip that computation. `average_weights` (see
[`MultiheadAttentionOptions`](MultiheadAttentionOptions.md)) only changes
whether those returned weights are merged across heads, never whether
they are computed at all.

## Signature

```cpp
explicit nn::MultiheadAttention::MultiheadAttention(nn::MultiheadAttentionOptions options);
nn::MultiheadAttention::MultiheadAttention(std::int64_t embed_dim, std::int64_t num_heads,
                                            double dropout = 0.0, bool bias = true,
                                            bool batch_first = false, bool average_weights = true,
                                            pluss::Config cfg = {}, std::uint64_t seed = 0);

void nn::MultiheadAttention::reset_parameters();
```

## Construction

Two equivalent forms: pass a
[`MultiheadAttentionOptions`](MultiheadAttentionOptions.md) directly, or
spell out the most-used fields as positional arguments. The positional
form has no parameter for `scale` -- reaching a non-default `scale`
requires the `MultiheadAttentionOptions` form:

```cpp
nn::MultiheadAttention a(nn::MultiheadAttentionOptions{4, 2});  // embed_dim=4, num_heads=2, everything else defaulted
nn::MultiheadAttention b(4, 2);                                 // equivalent positional form
```

`embed_dim` must be evenly divisible by `num_heads` -- constructing with
a mismatched pair throws immediately, before any weight is allocated.

## State

`options()` (mutable and `const` overloads, reachable only via `->`)
returns the [`MultiheadAttentionOptions`](MultiheadAttentionOptions.md)
this instance was constructed with.

`q_weight()` / `k_weight()` / `v_weight()` / `out_weight()` (mutable and
`const` overloads) -- `Series&` / `const Series&`
: The four learned projection matrices, each `[embed_dim, embed_dim]`.
  `q_weight`/`k_weight`/`v_weight` project the raw `query`/`key`/`value`
  inputs before the heads are split apart; `out_weight` projects the
  heads' concatenated output back to `embed_dim`. All four are registered
  parameters.

`q_bias()` / `k_bias()` / `v_bias()` / `out_bias()` (mutable and `const`
overloads) -- `Series&` / `const Series&`
: The four matching `[embed_dim]` bias vectors, registered as parameters
  when `options().bias == true`. Undefined `Series` values (not
  zero-filled ones) when `options().bias == false`.

## Methods

`forward(query, key, value, attn_mask = {}, key_padding_mask = {}, causal = false)` /
`operator()(...)` -- `tuple<Series, Series>`
: Runs multi-head scaled dot-product attention and returns
  `{output, weights}`. `query`, `key`, and `value` each accept rank 2
  (unbatched, `[seq_len, embed_dim]`) or rank 3 (batched, laid out per
  `options().batch_first`) input; `query`'s and `key`'s last axis must
  both equal `embed_dim`, and `key`'s and `value`'s sequence-length axis
  must match. `attn_mask` (boolean or additive, broadcastable to
  `[target_len, source_len]`) and `key_padding_mask` (boolean,
  `[source_len]` unbatched or `[batch, source_len]` batched, `true`
  meaning "ignore this key") independently block attention to specific
  positions; `causal` additionally blocks every query position from
  attending to any key position after it (standard autoregressive
  masking), and combines with either mask rather than replacing it. See
  Examples 3 and 4. `output` has the same rank and shape as `query`;
  `weights` has shape `[target_len, source_len]` (or `[batch, target_len,
  source_len]`) when `options().average_weights == true`, or
  `[num_heads, target_len, source_len]` (or `[batch, num_heads,
  target_len, source_len]`) when `false` -- see
  [`MultiheadAttentionOptions::average_weights`](MultiheadAttentionOptions.md#average_weights).

`reset_parameters()` -- `void`
: Re-fills all four weight matrices (and, when `options().bias == true`,
  all four bias vectors) in place with a fresh uniform draw in
  `[-1/sqrt(embed_dim), 1/sqrt(embed_dim)]`, using `options().seed`
  offset by a distinct constant per tensor. Called once during
  construction; call it again to reinitialize an already-trained
  instance. Never reassigns the weight/bias members themselves, so
  registration and gradient tracking are undisturbed across any number of
  calls.

## Practical Examples

Five examples: a hand-verified output and per-head weight averaging with
every projection set to identity, `average_weights=false` alongside the
positional-vs-options-struct construction equivalence, `causal=true`
masking future positions, a `key_padding_mask` blocking a specific key
position, and confirming `MultiheadAttention` runs on the MPS backend.

### Example 1

Every projection weight set to the identity matrix and every bias set to
zero, so `query == key == value == x` exactly and the output is
hand-verifiable against the plain scaled-dot-product softmax formula.
`embed_dim=4`, `num_heads=2` splits `x`'s 4 columns into two independent
2-wide heads.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // embed_dim=4, num_heads=2 -- head_dim=2
  nn::MultiheadAttention attn(4, 2);

  // set every projection to identity/zero by hand so the output is
  // hand-verifiable against the plain scaled-dot-product formula
  const pluss::Series identity4 = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0,
       0.0, 0.0, 0.0, 1.0},
      {4, 4});
  attn->q_weight().copy_(identity4);
  attn->k_weight().copy_(identity4);
  attn->v_weight().copy_(identity4);
  attn->out_weight().copy_(identity4);
  const pluss::Series zeros4 = pluss::series::zeros({4}, pluss::Config{});
  attn->q_bias().copy_(zeros4);
  attn->k_bias().copy_(zeros4);
  attn->v_bias().copy_(zeros4);
  attn->out_bias().copy_(zeros4);

  // unbatched input: [seq_len=2, embed_dim=4]
  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 4});

  const auto [output, weights] = attn.forward(x, x, x);

  std::cout << "output shape: [" << output.sizes()[0] << ", " << output.sizes()[1] << "]\n";
  std::cout << "weights shape (average_weights=true default): [" << weights.sizes()[0] << ", " << weights.sizes()[1] << "]\n";
  std::cout << "output: " << output << "\n";
  std::cout << "weights (averaged across the 2 heads): " << weights << "\n";
  std::cout << "weights row sums (each row is a softmax, must sum to 1): "
            << weights.sum(-1, false) << "\n";

  return 0;
}
```

Output

```text
output shape: [2, 4]
weights shape (average_weights=true default): [2, 2]
output: pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[0.66976155, 0.33023845, 0, 0],
  [0.33023845, 0.66976155, 0, 0]]
weights (averaged across the 2 heads): pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.58488077, 0.41511923],
  [0.41511923, 0.58488077]]
weights row sums (each row is a softmax, must sum to 1): pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Head 0 sees `x`'s first two columns (`[[1,0],[0,1]]` for both `q` and
`k`), giving logits `[[1/sqrt(2), 0], [0, 1/sqrt(2)]]` and a softmax of
`[0.6698, 0.3302]` per row -- exactly the first two output columns. Head
1 sees `x`'s last two columns, which are all zero, giving a uniform
`[0.5, 0.5]` softmax and a zero output (columns 3-4). The displayed
`weights` is the plain average of the two heads' softmax rows:
`(0.6698 + 0.5) / 2 = 0.58488...`.

### Example 2

`average_weights=false` keeps every head's weights separate instead of
averaging them, changing `weights`'s shape from `[target_len,
source_len]` to `[num_heads, target_len, source_len]`. Shown alongside
the `MultiheadAttentionOptions`-struct and positional constructor forms
producing identical options.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // positional constructor, average_weights=false
  nn::MultiheadAttention via_positional(4, 2, /*dropout=*/0.0, /*bias=*/true,
                                        /*batch_first=*/false, /*average_weights=*/false);

  // MultiheadAttentionOptions constructor, equivalent fields
  nn::MultiheadAttentionOptions opts{};
  opts.embed_dim = 4;
  opts.num_heads = 2;
  opts.average_weights = false;
  nn::MultiheadAttention via_options(opts);

  std::cout << "via_positional options: embed_dim=" << via_positional->options().embed_dim
            << ", num_heads=" << via_positional->options().num_heads
            << ", average_weights=" << std::boolalpha << via_positional->options().average_weights << "\n";
  std::cout << "via_options options:    embed_dim=" << via_options->options().embed_dim
            << ", num_heads=" << via_options->options().num_heads
            << ", average_weights=" << via_options->options().average_weights << "\n";

  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 4});

  const auto [output, weights] = via_positional.forward(x, x, x);
  std::cout << "average_weights=false weights shape: [" << weights.sizes()[0] << ", "
            << weights.sizes()[1] << ", " << weights.sizes()[2]
            << "] -- [num_heads, target_len, source_len], one matrix per head, not merged\n";

  return 0;
}
```

Output

```text
via_positional options: embed_dim=4, num_heads=2, average_weights=false
via_options options:    embed_dim=4, num_heads=2, average_weights=false
average_weights=false weights shape: [2, 2, 2] -- [num_heads, target_len, source_len], one matrix per head, not merged
```

### Example 3

`causal=true` blocks every query position from attending to any key
position after it -- the standard autoregressive mask. With identity
projections and 3 timesteps, position 0 collapses to attending only to
itself, while position 2 (the last) sees no blocking at all, since
nothing comes after it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::MultiheadAttention attn(4, 2);

  const pluss::Series identity4 = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0,
       0.0, 0.0, 0.0, 1.0},
      {4, 4});
  attn->q_weight().copy_(identity4);
  attn->k_weight().copy_(identity4);
  attn->v_weight().copy_(identity4);
  attn->out_weight().copy_(identity4);

  // 3 timesteps, unbatched
  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 4});

  const auto [out_full, w_full] = attn.forward(x, x, x);
  const auto [out_causal, w_causal] = attn.forward(x, x, x,
      pluss::series::undefined(), pluss::series::undefined(), /*causal=*/true);

  std::cout << "without causal, weights row 0 (attends to all 3 positions): " << w_full.reshape({9}).slice(0, 0, 3) << "\n";
  std::cout << "with causal=true, weights row 0 (position 0 can only attend to itself): "
            << w_causal.reshape({9}).slice(0, 0, 3) << "\n";
  std::cout << "with causal=true, weights row 2 (position 2 can attend to all 3, nothing is future): "
            << w_causal.reshape({9}).slice(0, 6, 9) << "\n";

  return 0;
}
```

Output

```text
without causal, weights row 0 (attends to all 3 positions): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.46778093, 0.2965803, 0.23563877]
with causal=true, weights row 0 (position 0 can only attend to itself): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
with causal=true, weights row 2 (position 2 can attend to all 3, nothing is future): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.34129582, 0.30260193, 0.35610226]
```

### Example 4

`key_padding_mask` independently blocks specific key positions from being
attended to at all, regardless of `causal`. A `true` entry means "ignore
this key" -- every query's weight on that column collapses to exactly
zero, and the remaining columns renormalize so each row still sums to 1.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::MultiheadAttention attn(4, 2);

  const pluss::Series identity4 = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0,
       0.0, 0.0, 0.0, 1.0},
      {4, 4});
  attn->q_weight().copy_(identity4);
  attn->k_weight().copy_(identity4);
  attn->v_weight().copy_(identity4);
  attn->out_weight().copy_(identity4);

  // unbatched, 1D key_padding_mask requires batch==1
  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 4});

  // mask out key position 1 (the middle timestep) -- true means "ignore this key"
  const pluss::Series key_padding_mask = pluss::make_series({false, true, false}, {3}).to(pluss::ScalarType::Bool);

  const auto [output, weights] = attn.forward(
      x, x, x, pluss::series::undefined(), key_padding_mask, /*causal=*/false);

  std::cout << "weights row 0 with key position 1 masked out: " << weights.reshape({9}).slice(0, 0, 3) << "\n";
  std::cout << "column 1 (the masked key) receives zero weight from every query\n";

  return 0;
}
```

Output

```text
weights row 0 with key position 1 masked out: pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.65751175, 0, 0.34248825]
column 1 (the masked key) receives zero weight from every query
```

### Example 5

`MultiheadAttention` runs on the MPS backend the same way it does on
CPU -- moving the module with `.to(pMPS)` moves every projection weight
and bias.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;
  using pluss::Series;

  nn::MultiheadAttention attn(4, 2);
  attn.to(pluss::pMPS);

  std::cout << "q_weight platform after .to(pMPS): " << static_cast<int>(attn->q_weight().platform()) << "\n";

  const Series x = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0}, {2, 4})
                        .to(pluss::ScalarType::Float32)
                        .to(pluss::pMPS);
  try {
    const auto [output, weights] = attn.forward(x, x, x);
    std::cout << "multihead attention on MPS: OK, output platform=" << static_cast<int>(output.platform()) << "\n";
  } catch (const std::exception& e) {
    std::cout << "multihead attention on MPS: FAILED -- " << e.what() << "\n";
  }

  return 0;
}
```

Output

```text
q_weight platform after .to(pMPS): 2
multihead attention on MPS: OK, output platform=2
```

## Changelog

- **0.1** — learned multi-head scaled dot-product attention, `embed_dim`/
  `num_heads`/`dropout`/`bias`/`batch_first`/`average_weights`/`scale`
  options; `attn_mask`/`key_padding_mask`/`causal` on `forward`.

## See Also

- [`MultiheadAttentionOptions`](MultiheadAttentionOptions.md) -- the
  options struct.
- [`TransformerEncoderLayer`](TransformerEncoderLayer.md) /
  [`TransformerDecoderLayer`](TransformerDecoderLayer.md) -- build and own
  one (or two) `MultiheadAttention` instances internally, reachable
  through [`self_attn()`](TransformerEncoderLayer.md#state) /
  [`cross_attn()`](TransformerDecoderLayer.md#state).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

MultiheadAttentionOptions
```
<!-- apiref:toctree:end -->
