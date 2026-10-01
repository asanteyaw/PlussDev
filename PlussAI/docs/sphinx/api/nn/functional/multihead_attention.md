# `pluss::ai::nn::multihead_attention`

Stateless multi-head attention over explicit projection weights; what
MultiheadAttention's forward calls internally.

## Signature

```cpp
std::tuple<Series, Series> nn::multihead_attention(
    const Series& query, const Series& key, const Series& value,
    const nn::MultiheadWeights& weights, nn::MultiheadOptions options = {},
    const Series& attn_mask = pluss::series::undefined(),
    const Series& key_padding_mask = pluss::series::undefined());
```

## What It Does

Splits `embed_dim` (`query`/`key`/`value`'s last axis) into
`options.num_heads` heads, runs [`scaled_attention`](scaled_attention.md)
independently per head, concatenates the heads' outputs back to
`embed_dim`, and applies `weights.out_weight`/`out_bias` -- exactly what
[`MultiheadAttention::forward`](../attention/MultiheadAttention.md#methods)
calls internally with that instance's own eight projection tensors (see
Example 1). `attn_mask`/`key_padding_mask` independently block specific
positions, identical in meaning to
[`MultiheadAttention::forward`](../attention/MultiheadAttention.md#methods)'s
own arguments of the same name; `options.attention.causal` additionally
blocks future positions, combining with (not replacing) either mask.

## Parameters

`const Series& query` / `const Series& key` / `const Series& value`
: Rank 2 (unbatched, `[seq_len, embed_dim]`) or rank 3 (batched, laid out
  per `options.batch_first`).

`const nn::MultiheadWeights& weights`
: See [`MultiheadWeights`](MultiheadWeights.md) -- the eight explicit
  projection weight/bias tensors.

`nn::MultiheadOptions options`
: See [`MultiheadOptions`](MultiheadOptions.md).

`const Series& attn_mask` / `const Series& key_padding_mask`
: Independently block specific positions. See
  [`MultiheadAttention::forward`](../attention/MultiheadAttention.md#methods)
  for exactly what each blocks.

## Returns

`{output, weights}`: `output` matches `query`'s own shape; `weights`'
shape depends on `options.average_weights` -- see
[`MultiheadOptions::average_weights`](MultiheadOptions.md#average_weights).

## Errors

- `query`'s/`key`'s last axis mismatched, `key`'s/`value`'s sequence
  length mismatched, or `embed_dim` not divisible by `options.num_heads`
  -- throws.

## Practical Examples

Three examples: a live `MultiheadAttention` instance's own projection
weights fed through this function matching `forward` exactly,
`average_weights=false` producing a per-head weight shape, and
`key_padding_mask` passed directly through this entry point blocking a
specific key position.

### Example 1

A live [`MultiheadAttention`](../attention/MultiheadAttention.md)
instance's own `q_weight()`/`k_weight()`/`v_weight()`/`out_weight()`/
`*_bias()`, bundled into a [`MultiheadWeights`](MultiheadWeights.md) and
fed into this function, produce bit-for-bit the same output and weights
as calling `forward` directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::MultiheadAttention attn(4, 2);
  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 4});

  const auto [module_output, module_weights] = attn.forward(x, x, x);

  nn::MultiheadWeights weights{};
  weights.q_weight = attn->q_weight();
  weights.k_weight = attn->k_weight();
  weights.v_weight = attn->v_weight();
  weights.out_weight = attn->out_weight();
  weights.q_bias = attn->q_bias();
  weights.k_bias = attn->k_bias();
  weights.v_bias = attn->v_bias();
  weights.out_bias = attn->out_bias();

  nn::MultiheadOptions options{};
  options.num_heads = 2;

  const auto [func_output, func_weights] = nn::multihead_attention(x, x, x, weights, options);

  std::cout << "identical output:  " << std::boolalpha << module_output.allclose(func_output) << "\n";
  std::cout << "identical weights: " << module_weights.allclose(func_weights) << "\n";

  return 0;
}
```

Output

```text
identical output:  true
identical weights: true
```

### Example 2

`options.average_weights=false` keeps every head's weights separate --
`[num_heads, target_len, source_len]` instead of the averaged
`[target_len, source_len]`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series identity4 = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0,
       0.0, 0.0, 0.0, 1.0},
      {4, 4});
  const pluss::Series zeros4 = pluss::series::zeros({4}, pluss::Config{});

  nn::MultiheadWeights weights{};
  weights.q_weight = identity4;
  weights.k_weight = identity4;
  weights.v_weight = identity4;
  weights.out_weight = identity4;
  weights.q_bias = zeros4;
  weights.k_bias = zeros4;
  weights.v_bias = zeros4;
  weights.out_bias = zeros4;

  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0},
      {2, 4});

  nn::MultiheadOptions opts{};
  opts.num_heads = 2;
  opts.average_weights = false;

  const auto [output, weights_out] = nn::multihead_attention(x, x, x, weights, opts);

  std::cout << "average_weights=false shape: [" << weights_out.sizes()[0] << ", "
            << weights_out.sizes()[1] << ", " << weights_out.sizes()[2]
            << "] -- [num_heads, target_len, source_len]\n";

  return 0;
}
```

Output

```text
average_weights=false shape: [2, 2, 2] -- [num_heads, target_len, source_len]
```

### Example 3

`key_padding_mask`, passed directly as this function's seventh
argument, blocks a specific key position from being attended to by
every query -- the masked column's weight collapses to exactly zero.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series identity4 = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0,
       0.0, 0.0, 0.0, 1.0},
      {4, 4});
  const pluss::Series zeros4 = pluss::series::zeros({4}, pluss::Config{});

  nn::MultiheadWeights weights{};
  weights.q_weight = identity4;
  weights.k_weight = identity4;
  weights.v_weight = identity4;
  weights.out_weight = identity4;
  weights.q_bias = zeros4;
  weights.k_bias = zeros4;
  weights.v_bias = zeros4;
  weights.out_bias = zeros4;

  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 4});

  const pluss::Series key_padding_mask = pluss::make_series({false, true, false}, {3}).to(pluss::ScalarType::Bool);

  nn::MultiheadOptions opts{};
  opts.num_heads = 2;

  const auto [output, weights_out] = nn::multihead_attention(
      x, x, x, weights, opts, pluss::series::undefined(), key_padding_mask);

  std::cout << "weights row 0 with key position 1 masked out: " << weights_out.reshape({9}).slice(0, 0, 3) << "\n";

  return 0;
}
```

Output

```text
weights row 0 with key position 1 masked out: pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.58488077, 0, 0.41511923]
```

## See Also

- [`MultiheadOptions`](MultiheadOptions.md) /
  [`MultiheadWeights`](MultiheadWeights.md) -- the options and weight
  types this function takes.
- [`scaled_attention`](scaled_attention.md) -- run once per head
  internally.
- [`MultiheadAttention`](../attention/MultiheadAttention.md) -- the
  stateful `Module` wrapper this function powers.
