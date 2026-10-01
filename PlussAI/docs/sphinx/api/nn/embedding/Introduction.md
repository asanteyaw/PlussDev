# `nn::embedding` Introduction

## What This Category Owns

`embedding` owns the layer that converts a categorical integer code into a
dense, learned vector:

- [`Embedding`](Embedding.md) -- a lookup table, one row per category,
  gathered by index. Any integer-valued `Series` shape is a valid input;
  the output is that same shape with a `hidden_size` axis appended. The
  only type in this category so far.

Nothing else in this category exists yet -- there is exactly one lookup
primitive underneath (`Series::index_select`), and `Embedding` is its sole
`Module` wrapper. A future variant that pools several codes per row into
one vector (a "bag" of embeddings, averaged or summed rather than looked
up one at a time) is not present.

## Why A Lookup Table, Not A One-Hot Encoding

Feeding a raw category code directly into a linear layer imposes a false
ordering (`Fusion`, like any linear layer, treats input magnitude as
meaningful, but "category 7" is not numerically "more" than "category 3").
The classical fix, one-hot encoding, avoids the false ordering but wastes
computation and memory at any real `cardinality` -- a `[batch,
cardinality]` matrix that is `1` in exactly one place per row. `Embedding`
gets the same "no false ordering" property directly: each category's
vector is learned independently, with no numerical relationship implied
by adjacent category codes, and the lookup itself costs nothing beyond
selecting `hidden_size` values per row.

## Parameters: One, Like `PReLU`

`Embedding` has one learnable parameter, `weight` -- the same parameter
count as [`PReLU`](../activation/PReLU.md), though its shape (`[cardinality,
hidden_size]`, one full row per category) is far larger than PReLU's
per-channel slope for any realistic `cardinality`.

## Choosing `hidden_size`

There is no formula that picks `hidden_size` automatically. A reasonable
starting point:

1. For a small `cardinality` (a handful of categories), a small
   `hidden_size` (4-8) is usually enough -- there just isn't much
   structure to capture with only a few distinct categories.
2. For a large `cardinality` (thousands of categories, such as product IDs
   or vocabulary tokens), a larger `hidden_size` (16-256, depending on the
   downstream task) gives the table room to place semantically similar
   categories near each other in the embedding space during training.
3. Always place `Embedding` as (or near) the first stage in a
   [`Schedule`](../module/Schedule.md) -- it expects raw integer category
   codes as input, not the continuous features every other layer in this
   reference produces or consumes.

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract `Embedding` implements, and how layers compose into a
  `Schedule`.
- [`normalization`](../normalization.md) -- the other family whose only
  member requires an explicit options argument with no sensible default.
- [`activation`](../activation.md) -- the elementwise nonlinearities
  typically placed after an `Embedding` lookup.
