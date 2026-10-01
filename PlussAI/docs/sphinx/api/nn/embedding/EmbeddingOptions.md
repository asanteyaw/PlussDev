# `pluss::ai::nn::EmbeddingOptions`

The table's category cardinality, embedding width, and where its learned
weight is allocated.

Passed to [`Embedding`](Embedding.md)'s constructor. Like
[`LayerNormOptions`](../normalization/LayerNormOptions.md),
`cardinality`/`hidden_size` have no meaningful default (`0` is not a usable
table shape) -- `Embedding` has no no-argument constructor, so an
`EmbeddingOptions` with both set must always be passed explicitly.

## Fields

### `cardinality`

`std::int64_t cardinality = 0`
: The number of distinct category codes the table holds one row for --
  `weight`'s first dimension. Every index `forward` receives must be an
  integer in `[0, cardinality)`; an out-of-range index throws cleanly
  rather than reading out of bounds (see [`Embedding`](Embedding.md)'s
  Errors section).

### `hidden_size`

`std::int64_t hidden_size = 0`
: The width of each category's embedding vector -- `weight`'s second
  dimension, and the size of the trailing axis `forward` appends to
  whatever shape `index` has.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform `weight` is allocated with. Defaults to the
  engine's default `Config` (Float32 on CPU, the same default every other
  parameterized layer in this reference uses). A non-floating `cfg.dtype`
  is silently promoted to Float32 before allocation -- a category
  embedding table is meaningless in an integer dtype, so there is no
  floating/non-floating choice to preserve here the way there might be
  elsewhere.

### `seed`

`std::uint64_t seed = 0`
: The seed behind the table's random initialization (and any later
  [`reset_parameters()`](Embedding.md) call). The same `seed` on the same
  `cardinality`/`hidden_size`/`cfg` always reproduces the exact same
  initial weights.

## Guidance

Set `cardinality` to the number of distinct categories in your data
(typically the output of a category-encoding step upstream) and
`hidden_size` to whatever embedding width your architecture calls for --
there is no formula that picks it automatically. Leave `cfg` at its
default and use `.to(...)` on the constructed module for platform/dtype
changes. Set `seed` explicitly only when exact reproducibility across runs
matters; leaving it at `0` is fine otherwise, since `0` is just as valid a
seed as any other.

## See Also

- [`Embedding`](Embedding.md) -- the only consumer of this options struct.
