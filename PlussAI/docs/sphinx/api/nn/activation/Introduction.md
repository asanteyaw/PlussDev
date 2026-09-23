# `nn::activation` Introduction

## What This Category Owns

`activation` owns the elementwise nonlinearities placed between linear
stages ([`fusion`](../fusion.md), [`cnn`](../cnn.md), [`rnn`](../rnn.md)) so
a network can represent more than one big linear map:

- [`ReLU`](ReLU.md) -- the default choice: `max(0, x)`, cheap, and its
  gradient is either exactly `0` or exactly `1`.
- [`Sigmoid`](Sigmoid.md) / [`Tanh`](Tanh.md) -- the classical squashing
  functions, `(0, 1)`- and `(-1, 1)`-ranged respectively. Mostly reached for
  today as a final layer (`Sigmoid`, for a probability) or inside gate
  formulations (both, inside recurrent cells), rather than as a general
  hidden-layer activation -- `ReLU`-family activations dominate there because
  `Sigmoid`/`Tanh`'s gradient vanishes for large-magnitude inputs.
- [`Softplus`](Softplus.md) / [`LeakyReLU`](LeakyReLU.md) / [`Elu`](Elu.md)
  -- three different answers to `ReLU`'s one real weakness (a negative input
  produces exactly zero gradient, so a unit that only ever sees negative
  pre-activations can stop learning entirely): `Softplus` fixes it by never
  being exactly flat anywhere, `LeakyReLU` by keeping a small linear leak,
  `Elu` by keeping a small, smooth, saturating leak.
- [`PReLU`](PReLU.md) -- the same answer as `LeakyReLU`, but the leak's
  slope is a learned parameter instead of a fixed constant.
- [`SiLU`](SiLU.md) / [`GELU`](GELU.md) -- smooth, non-monotonic modern
  alternatives to `ReLU` (each dips slightly below `0` for small negative
  inputs before flattening), common in transformer and other contemporary
  architectures. `GELU` in particular is the conventional choice inside
  transformer feed-forward blocks.
- [`Hardshrink`](Hardshrink.md) -- not a squashing function at all: zeroes a
  band around `0` and passes everything else through completely unchanged,
  used for sparsifying activations rather than reshaping their range.
- [`Softmax`](Softmax.md) / [`LogSoftmax`](LogSoftmax.md) -- turn a layer of
  raw logits into a per-axis probability distribution (or its log), the
  standard final transformation before a multi-class classification loss.
- [`Identity`](Identity.md) -- not an activation in the mathematical sense
  (it computes nothing), but grouped here because it fills the same
  structural role: one stage in a `Schedule`, standing in for "no
  activation" so calling code never special-cases that choice.

## Parameters: One Exception

Every type in this category adds nothing to a `Schedule`'s
[`parameters()`](../module/Module/parameters.md) except
[`PReLU`](PReLU.md), whose negative-side slope is a learned weight rather
than a fixed constant. `train()`/`eval()` still propagate to every type here
regardless (some future activation could plausibly have training-mode
behavior beyond `PReLU`'s learned weight; none of the others do), but only
`PReLU` gives [`optim`](../optim.md) anything to step.

## Choosing One

There is no universally correct choice, but a reasonable default path:

1. Start with [`ReLU`](ReLU.md) for hidden layers. It is the cheapest and,
   empirically, trains reliably for most architectures.
2. If training diagnostics show units going permanently inactive ("dying
   ReLU"), reach for [`LeakyReLU`](LeakyReLU.md) first (cheap, one extra
   multiply) before [`Elu`](Elu.md) (one extra `exp` call, but a smooth
   rather than piecewise-linear negative side).
3. Reach for [`Sigmoid`](Sigmoid.md) specifically as a final layer for
   binary classification -- and prefer computing raw logits and calling
   [`losses::BCEWithLogits`](../losses/BCEWithLogits.md) directly over
   applying `Sigmoid` and a separate cross-entropy afterward, for numerical
   stability.
4. Reach for [`Tanh`](Tanh.md) when a zero-centered bounded output
   specifically matters for what follows -- most notably inside `RNN`'s
   default nonlinearity and `GRU`/`LSTM`'s internal gate formulations.
5. Reach for [`GELU`](GELU.md) (or [`SiLU`](SiLU.md)) instead of `ReLU` when
   following a modern architecture family that specifies one of them --
   transformer feed-forward blocks conventionally use `GELU`.
6. Reach for [`PReLU`](PReLU.md) over `LeakyReLU` only when the extra
   trainable parameter is worth the added complexity for the architecture
   at hand; `LeakyReLU`'s fixed slope is the simpler default.
7. Reach for [`Hardshrink`](Hardshrink.md) specifically to sparsify
   activations by zeroing a band around `0`, not as a general-purpose
   hidden-layer activation.
8. Reach for [`Softmax`](Softmax.md) / [`LogSoftmax`](LogSoftmax.md) as the
   final layer for multi-class classification -- and prefer computing raw
   logits and calling a cross-entropy loss directly over applying `Softmax`
   and a separate loss afterward, for the same numerical-stability reason
   `Sigmoid` + `BCEWithLogits` are preferred over `Sigmoid` alone.
9. Reach for [`Identity`](Identity.md) only as the "no activation" branch of
   conditional architecture code, never as a considered activation choice
   in its own right.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module` contract every
  type here implements, and how layers compose into a `Schedule`.
- [`fusion`](../fusion.md), [`cnn`](../cnn.md), [`rnn`](../rnn.md),
  [`attention`](../attention.md) -- the layer types activations are placed
  between.
- [`functional`](../functional.md) -- `Elu`'s stateless
  [`elu`](../functional/elu.md) function form.
