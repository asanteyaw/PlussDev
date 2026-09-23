# `pluss::ai::nn::GELUOptions`

Selects between the exact and the tanh-approximated formula.

Passed to [`GELU`](GELU.md)'s constructor; has one field with a default, so
`GELUOptions{}` (or omitting the argument entirely) reproduces the exact
curve.

## Fields

### `approximate`

`bool approximate = false`
: When `false` (the default), `forward` computes the exact formula
  `0.5 * x * (1 + erf(x / sqrt(2)))`. When `true`, it instead evaluates the
  faster tanh-based approximation historically used before fast `erf`
  implementations were common. The two agree to several decimal places
  everywhere and are visually indistinguishable; they are not bit-identical.

## Guidance

Leave `approximate` at its default `false` unless profiling shows `erf`
itself is a measurable bottleneck at your model's scale -- the tanh
approximation exists for that case, not as a general-purpose default.
Mixing the two within a single trained model (e.g. exact at training time,
approximate at inference time) will shift outputs slightly; keep it
consistent across a model's lifetime.

## See Also

- [`GELU`](GELU.md) -- the only consumer of this options struct.
