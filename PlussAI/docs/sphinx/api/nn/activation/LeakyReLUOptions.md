# `pluss::ai::nn::LeakyReLUOptions`

Controls the slope applied to negative inputs.

Passed to [`LeakyReLU`](LeakyReLU.md)'s constructor; has one field with a
default, so `LeakyReLUOptions{}` (or omitting the argument entirely)
reproduces the standard `0.01` leak.

## Fields

### `negative_slope`

`double negative_slope = 0.01`
: The multiplier applied to a negative input (`x >= 0 ? x : negative_slope *
  x`). `0.0` reproduces plain [`ReLU`](ReLU.md) exactly; `1.0` reproduces
  the identity function (no clipping at all). Values in between trade off
  how much negative-side signal and gradient survive against how much the
  layer still behaves like a rectifier.

## Guidance

The default `0.01` is the conventional small leak from the original paper --
enough to avoid dead units without meaningfully changing the layer's
rectifying behavior. Raise it toward `0.1`–`0.3` if training diagnostics
show units going permanently inactive; there is rarely a reason to go much
higher without switching to a different activation entirely (see
[`Elu`](Elu.md) for a smooth alternative).

## See Also

- [`LeakyReLU`](LeakyReLU.md) -- the only consumer of this options struct.
