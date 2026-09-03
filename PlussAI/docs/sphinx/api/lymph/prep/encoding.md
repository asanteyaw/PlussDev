# Lymph `prep::encoding`

`encoding` is the `prep` subfamily of **standalone, fittable categorical
encoders** — objects you construct, `learn()` on a frame, and then `map()` over
as many later frames as you like, with the fitted category layout held on the
object and readable through `summary()` and the public attributes.

Each encoder here is the object twin of a
[`prep::stage`](stage_family.md) factory: `prep::OneHotEncoder` does the same
arithmetic as `prep::stage::one_hot_encoder(...)`, from the same
[`options::OneHotEncoder`](encoding/OneHotEncoderOptions.md) struct. The
difference is ownership.

| Reach for… | when… |
| --- | --- |
| the [`prep::stage::*`](stage_family.md) factory | the encoding should travel *inside a model's* preparation plan and be fitted by `Model::learn` |
| the object here | the encoder is a *first-class thing you hold* — to fit once and reuse across frames, to read its learned category set, or to wire into a `prep::FeatureTransformer` branch or non-model code |

## The encoders

| Object | Options | What `map` produces |
| --- | --- | --- |
| [`prep::OneHotEncoder`](encoding/OneHotEncoder.md) | [`options::OneHotEncoder`](encoding/OneHotEncoderOptions.md) | one `0/1` indicator column per learned category, plus a reserved `_unknown` column — **widens the frame** |
| [`prep::OrdinalEncoder`](encoding/OrdinalEncoder.md) | [`options::OrdinalEncoder`](encoding/OrdinalEncoderOptions.md) | one integer code per row, **1-based** (0 = unseen) — same column, same name |
| [`prep::LabelEncoder`](encoding/LabelEncoder.md) | [`options::LabelEncoder`](encoding/LabelEncoderOptions.md) | one integer code per row, **0-based** dense `0..K-1` — same column |
| [`prep::FrequencyEncoder`](encoding/FrequencyEncoder.md) | [`options::FrequencyEncoder`](encoding/FrequencyEncoderOptions.md) | each category replaced by its observed relative frequency in `[0, 1]` — same column |
| [`prep::BaseNEncoder`](encoding/BaseNEncoder.md) | [`options::BaseNEncoder`](encoding/BaseNEncoderOptions.md) | the category's ordinal code written in radix `base` across `ceil(log_base(K+1))` digit columns — widens the frame |
| [`prep::HashingEncoder`](encoding/HashingEncoder.md) | [`options::HashingEncoder`](encoding/HashingEncoderOptions.md) | an `n_components`-wide indicator from a fixed hash — width independent of cardinality, no vocabulary stored |

The supervised pair — **target encoding** and **weight-of-evidence** — take the
label at fit time, so they exist only as stage factories, not as standalone
objects: see [`prep::stage::target_encoder`](stage/target_encoder.md) and
[`prep::stage::woe_encoder`](stage/woe_encoder.md).

## The shared lifecycle

All six have the same core method surface — documented in full, with a worked
example per method, on [`prep::OneHotEncoder`](encoding/OneHotEncoder.md):

- [`features({...})`](encoding/OneHotEncoder.md#features) — pin the categorical
  columns to act on. **Required** on the `DataFrame` path — an encoder with no
  pinned columns raises rather than guessing which columns are categorical.
- [`learn(frame)`](encoding/OneHotEncoder.md#learn) — record the category set of
  each pinned column. Returns `*this`, so
  `encoder.features({...}).learn(frame)` chains.
- [`map(frame)`](encoding/OneHotEncoder.md#map) — replay the fitted layout onto a
  frame, returning the transformed copy.
- [`learn_map(frame)`](encoding/OneHotEncoder.md#learn_map) — `learn` then `map`
  in one call.
- [`learned()`](encoding/OneHotEncoder.md#learned) — has `learn` run yet?
- [`summary()`](encoding/OneHotEncoder.md#summary) — a `TransformerSummary`
  (`feature_names` plus a `state` tensor vector).

`OneHotEncoder`, `OrdinalEncoder`, `LabelEncoder` and `FrequencyEncoder` also
carry **single-column overloads** — pass one `Thread`, get one `Series` back,
with no `features()` pin — and expose the learned layout through the public
`categories_` / `mapped_names_` attributes.

## Unknown categories at replay time

A category not seen at `learn` time is an *unknown*. Whether the reserved bucket
is created is a fit-time choice (`include_unknown_bucket`); what `map` **does**
with an unknown is a separate replay-time choice carried on
[`InferOptions::unknown_categorical`](encoding/OneHotEncoder.md#map), and it
defaults to `Error`. Pass `MissingBucket` to route unknowns to the reserved
column / code, or `Ignore` for an all-zero row. `HashingEncoder` is stateless —
an unknown category simply hashes somewhere, no policy needed.

## String vs. integer-coded categories

Every encoder here accepts either an integer-coded column or a text
(note-backed) column. On the `DataFrame` path a text column's category identity
is taken as a **hash**, so `mapped_names_` reads `region_<hash>`; feed
integer-coded categories, or use the single-column `Thread` overload, to keep
readable labels.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

encoding/Introduction
encoding/OneHotEncoder
encoding/OrdinalEncoder
encoding/LabelEncoder
encoding/FrequencyEncoder
encoding/BaseNEncoder
encoding/HashingEncoder
```
<!-- apiref:toctree:end -->
