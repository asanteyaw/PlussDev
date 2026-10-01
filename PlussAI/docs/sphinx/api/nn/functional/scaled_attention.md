# `pluss::ai::nn::scaled_attention`

Stateless scaled dot-product attention over query/key/value `Series`,
returning output and attention weights.

## Signature

```cpp
std::tuple<Series, Series> nn::scaled_attention(
    const Series& query, const Series& key, const Series& value,
    const Series& attn_mask = pluss::series::undefined(),
    nn::AttentionOptions options = {});
```

## What It Does

Computes `softmax((query @ key^T) * scale + mask) @ value`, returning
`{output, weights}` -- the single-head attention primitive
[`multihead_attention`](multihead_attention.md) runs once per head
internally (see Example 3: a 1-head `MultiheadAttention` with identity
projections reduces to exactly this call on the raw inputs). `scale`
defaults to `1 / sqrt(head_dim)` (the last axis of `query`), or an
explicit value via `options.scale`. Unlike
[`MultiheadAttention`](../attention/MultiheadAttention.md), there is no
learned projection here at all -- `query`/`key`/`value` are used
directly, with no `q_weight`/`k_weight`/`v_weight`/`out_weight` in the
picture.

## Parameters

`const Series& query` / `const Series& key` / `const Series& value`
: Rank `>= 2`, `[..., target_len, head_dim]` / `[..., source_len,
  head_dim]` / `[..., source_len, head_dim]`. `query`'s and `key`'s last
  axis must match; `key`'s and `value`'s second-to-last axis
  (sequence length) must match.

`const Series& attn_mask`
: Boolean or additive, broadcastable against the `[..., target_len,
  source_len]` logits. Combines with (does not replace)
  `options.causal`.

`nn::AttentionOptions options`
: See [`AttentionOptions`](AttentionOptions.md).

## Returns

`{output, weights}`: `output` has `query`'s own shape; `weights` has
shape `[..., target_len, source_len]`.

## Errors

- `query`'s/`key`'s last axis mismatched, or `key`'s/`value`'s sequence
  length mismatched -- throws.
- `options.dropout` outside `[0, 1]` -- throws.

## Practical Examples

Three examples: a hand-verified result with identity-like query/key,
`causal=true` masking and an explicit `scale` overriding the default,
and confirming a 1-head `MultiheadAttention` with identity projections
reduces to exactly this function called on the raw inputs.

### Example 1

With `query == key` orthogonal-ish rows, the softmax weights are
hand-verifiable directly from the scaled dot-product formula.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <cmath>

int main() {
  namespace nn = pluss::ai::nn;

  // 2 timesteps, head_dim=2 -- identity-like q/k/v
  const pluss::Series q = pluss::make_series({1.0, 0.0, 0.0, 1.0}, {2, 2});
  const pluss::Series k = pluss::make_series({1.0, 0.0, 0.0, 1.0}, {2, 2});
  const pluss::Series v = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {2, 2});

  const auto [output, weights] = nn::scaled_attention(q, k, v);

  std::cout << "output: " << output << "\n";
  std::cout << "weights: " << weights << "\n";

  // hand check: logits = q@k^T * (1/sqrt(2)) = identity*0.7071
  const double scale = 1.0 / std::sqrt(2.0);
  const double diag_logit = scale;
  const double off_logit = 0.0;
  const double e_diag = std::exp(diag_logit);
  const double e_off = std::exp(off_logit);
  const double w_diag = e_diag / (e_diag + e_off);
  const double w_off = e_off / (e_diag + e_off);
  std::cout << "hand check weights[0]: [" << w_diag << ", " << w_off << "]\n";

  return 0;
}
```

Output

```text
output: pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[16.604769, 26.604769],
  [23.395231, 33.395231]]
weights: pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.66976155, 0.33023845],
  [0.33023845, 0.66976155]]
hand check weights[0]: [0.669762, 0.330238]
```

### Example 2

`options.causal=true` collapses row 0's weights to attend only to
itself; an explicit `options.scale=1.0` (no scaling at all) produces
genuinely different weights than the default `1/sqrt(head_dim)` scale.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series q = pluss::make_series({1.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {3, 2});
  const pluss::Series k = q;
  const pluss::Series v = q;

  nn::AttentionOptions opts{};
  opts.causal = true;
  const auto [output, weights] = nn::scaled_attention(q, k, v, pluss::series::undefined(), opts);

  std::cout << "causal weights row 0 (only attends to itself): " << weights.reshape({9}).slice(0, 0, 3) << "\n";

  // explicit scale overrides the default 1/sqrt(head_dim)
  nn::AttentionOptions custom_scale{};
  custom_scale.scale = 1.0;  // no scaling at all
  const auto [output2, weights2] = nn::scaled_attention(q, k, v, pluss::series::undefined(), custom_scale);
  const auto [output_default, weights_default] = nn::scaled_attention(q, k, v);
  std::cout << "scale=1.0 differs from the default 1/sqrt(head_dim) scale: "
            << std::boolalpha << !weights2.allclose(weights_default) << "\n";

  return 0;
}
```

Output

```text
causal weights row 0 (only attends to itself): pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
scale=1.0 differs from the default 1/sqrt(head_dim) scale: true
```

### Example 3

A 1-head [`MultiheadAttention`](../attention/MultiheadAttention.md)
with every projection set to identity has nothing left to project --
its output matches this function called directly on the raw `q`/`k`/`v`
values, confirming `scaled_attention` genuinely is the per-head
primitive `multihead_attention` builds on.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // num_heads=1 with identity projections reduces MultiheadAttention to
  // plain scaled_attention on the raw q/k/v
  nn::MultiheadAttention attn(4, 1);
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

  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 4});

  const auto [mha_output, mha_weights] = attn.forward(x, x, x);
  const auto [sa_output, sa_weights] = nn::scaled_attention(x, x, x);

  std::cout << "MultiheadAttention (1 head, identity projections) == scaled_attention directly: "
            << std::boolalpha << mha_output.allclose(sa_output) << "\n";

  return 0;
}
```

Output

```text
MultiheadAttention (1 head, identity projections) == scaled_attention directly: true
```

## See Also

- [`AttentionOptions`](AttentionOptions.md) -- the options struct.
- [`multihead_attention`](multihead_attention.md) -- runs this once per
  head, with learned projections around it.
- [`MultiheadAttention`](../attention/MultiheadAttention.md) -- the
  stateful `Module` wrapper built on `multihead_attention`.
