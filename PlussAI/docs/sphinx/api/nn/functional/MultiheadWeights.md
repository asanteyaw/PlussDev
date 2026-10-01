# `pluss::ai::nn::MultiheadWeights`

The explicit query/key/value/output projection weights and biases
`multihead_attention` takes directly.

## Signature

```cpp
Series q_weight;
Series k_weight;
Series v_weight;
Series out_weight;
Series q_bias;
Series k_bias;
Series v_bias;
Series out_bias;
```

A plain aggregate -- there is no constructor beyond the implicit
default/aggregate ones. The four `*_bias` fields may be left undefined
to skip that projection's bias addition entirely, the same convention
[`MultiheadAttention`](../attention/MultiheadAttention.md)'s own
`options().bias == false` case uses.

## State

`q_weight` / `k_weight` / `v_weight` / `out_weight` -- `Series`
: Each `[embed_dim, embed_dim]`. `q_weight`/`k_weight`/`v_weight`
  project the raw `query`/`key`/`value` inputs before the heads are
  split apart; `out_weight` projects the heads' concatenated output
  back to `embed_dim`.

`q_bias` / `k_bias` / `v_bias` / `out_bias` -- `Series`
: Each `[embed_dim]`, or an undefined `Series` to skip that projection's
  bias addition.

## Methods

None -- a plain data aggregate; every operation that consumes it is a
free function ([`multihead_attention`](multihead_attention.md)), never a
member of this type itself.

## Changelog

- **0.1** — the explicit four-projection weight/bias bundle
  `multihead_attention` takes directly.

## See Also

- [`multihead_attention`](multihead_attention.md) -- the only consumer
  of this type.
- [`MultiheadAttention`](../attention/MultiheadAttention.md) -- the
  stateful `Module` wrapper whose own
  `q_weight()`/`k_weight()`/`v_weight()`/`out_weight()`/`*_bias()`
  accessors return exactly the values this type's fields hold.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
