# `pluss::ai::classic::prep::BaseNEncoderOptions`

The digit radix, vocabulary cap, and overflow policy consumed by
[`BaseNEncoder::learn`](BaseNEncoder.md) / `map`.

## Fields

### `base`

`std::int64_t base = 2`

The radix each fitted code is expanded into digits of. `2` (the default) is
the classic "binary encoder." Must be `>= 2`; `learn` rejects anything
smaller with `Status::invalid_argument`. A larger base fits the same
vocabulary into fewer output columns (each column carries more information)
at the cost of a less sparse, less directly interpretable encoding.

### `max_categories`

`std::int64_t max_categories = 1024`

The largest vocabulary `learn` will fit per feature before `overflow`
applies. `0` means unlimited. Must be `>= 0`. The highest default among the
category-vocabulary encoders in this sub-family, since `BaseNEncoder`'s
output width grows only logarithmically with cardinality rather than
linearly — a large vocabulary is comparatively cheap here.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

As [`OneHotEncoderOptions::overflow`](OneHotEncoderOptions.md#overflow). A
category dropped this way collapses to the same code `0` a genuinely unseen
value gets at `map` time — see [`BaseNEncoder`](BaseNEncoder.md)'s own
`## Methods` section.

## Guidance

`base = 2` is the right default for most uses; raise it only when column
count is under real pressure and you are willing to trade sparsity and
interpretability for a narrower output. `max_categories` rarely needs
lowering from its default given how slowly the output width grows with
vocabulary size — the more common reason to touch it is capping runaway
cardinality itself, not the resulting column count.

## See Also

- [`BaseNEncoder`](BaseNEncoder.md) — the consuming type.
- [`HashingEncoder`](HashingEncoder.md) — a fixed-width alternative that
  needs no vocabulary or cap at all.
- [`OneHotEncoderOptions`](OneHotEncoderOptions.md) — the full
  `CategoryOverflowPolicy` documentation.
