# `nn::functional` Introduction

## What This Category Owns

`functional` owns the stateless free-function surface: exactly what
every stateful `Module` class elsewhere in this reference calls
internally, exposed directly for callers who want the computation
without constructing or holding a `Module`. Four groups:

- **Plain building blocks** -- [`fusion`](fusion.md), [`dropout`](dropout.md),
  [`elu`](elu.md).
- **Convolution** -- [`conv`](conv.md) (with the `conv2d`/`conv3d` fixed-rank
  forms), [`conv1d`](conv1d.md), and [`causal_padding1d`](causal_padding1d.md).
- **Recurrence** -- [`rnn`](rnn.md), [`gru`](gru.md), [`lstm`](lstm.md),
  each over explicit [`NetWeight`](NetWeight.md)/[`NetConfig`](NetConfig.md)
  rather than a constructed `Module`'s own registered parameters.
- **Attention** -- [`scaled_attention`](scaled_attention.md),
  [`multihead_attention`](multihead_attention.md) (over explicit
  [`MultiheadWeights`](MultiheadWeights.md)), and the mask builders
  [`causal_attention_mask`](causal_attention_mask.md)/
  [`square_subsequent_mask`](square_subsequent_mask.md).

Every function here is verified, in this reference's own examples, to
produce bit-for-bit identical results to the corresponding `Module`
class's `forward` -- there is exactly one implementation of each
computation; the `Module` classes are a thin, parameter-owning shell
around these same functions, not a separate reimplementation.

## The `pluss::nn` Short Alias

Everything in this family -- and every `Module` class throughout this
whole reference -- is additionally reachable through a short
`pluss::nn::*` alias (`namespace nn = pluss::ai::nn;`, defined in
`<pluss/nn.h>`), alongside the fully-qualified `pluss::ai::nn::*`
spelling every example in this reference uses. Both spellings name the
exact same functions and types.

## Function vs `Module`: When To Reach For Which

Reach for a function in this family directly when:

- The weights already exist as plain `Series` values from some other
  source (loaded from a file, produced by another computation), and
  constructing a full `Module` around them would be unnecessary
  ceremony.
- Building a custom composite operation that needs one of these
  computations as an internal step, without wanting to register or own
  a whole `Module`'s worth of parameters for it.
- Working entirely outside the `Module`/`Schedule` composition system --
  no `parameters()`, no `train()`/`eval()`, no `module_record()`.

Reach for the corresponding `Module` class instead when the weights need
to be **owned**: allocated, initialized, trained (registered so an
optimizer can find them), saved/restored, and moved as a unit between
platforms or dtypes via `.to(...)`. Every doc page in this family links
to its own `Module` counterpart, and every counterpart's own examples
demonstrate the two producing identical results.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract these functions sit underneath.
- [`fusion`](../fusion.md) / [`cnn`](../cnn.md) / [`rnn`](../rnn.md) /
  [`attention`](../attention.md) -- the stateful `Module` families this
  family's functions power.
