# `pluss::ai::ice::models::TextJudgeOptions`

Hidden width, dropout, and tokenization toggles (lemmas/punctuation/
casing) for [`TextJudge`](TextJudge.md)'s constructor.

## Fields

### `width`

`std::int64_t width = 64`
: Hidden width of the text classifier.

### `dropout`

`double dropout = 0.1`
: Dropout strength, fixed at construction. Overridable per training
  run via
  [`TextJudgeTrainOptions::architecture_regularization.dropout`](TextJudgeTrainOptions.md)
  -- see
  [`ArchitectureRegularization`'s own Guidance](../options/ArchitectureRegularization.md)
  for exactly how the two interact.

### `use_lemmas`

`bool use_lemmas = true`
: Tokenize on lemmas rather than raw surface forms.

### `skip_punct`

`bool skip_punct = true`
: Drop punctuation tokens before they reach the model. Set to `false`
  when punctuation itself is a meaningful signal -- see
  [`TextJudge::learn`'s own Example 3](TextJudge/learn.md#example-3)
  for a label pair that is only distinguishable by punctuation.

### `lowercase`

`bool lowercase = true`
: Lowercase every token before tokenization.

### `cfg`

`Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU)`
: Dtype and platform the model's own parameters are constructed on.

### `seed`

`std::uint64_t seed = 0`
: Seeds the model's own parameter initialization -- independent of
  [`TextJudgeTrainOptions::seed`](TextJudgeTrainOptions.md), which
  seeds shuffling/the train-validation split instead.

## Guidance

Leave every field at its default for a first run against clean,
casing-insensitive English text. Set `use_lemmas = false`/
`skip_punct = false`/`lowercase = false` individually when the task's
labels actually depend on surface form, punctuation, or casing --
see [`TextJudge::learn`'s own Example 3](TextJudge/learn.md#example-3)
for a worked punctuation-sensitive case. Unlike every `models_ts`
catalog entry's own `Options`, `TextJudgeOptions` has no `from_data`/
`from_dataset` factory -- `TextJudge` sizes itself from whatever text
it sees at `learn()` time instead of from a pre-built dataset.

## See Also

- [`TextJudge`](TextJudge.md) -- the constructor this configures.
- [`TextJudgeTrainOptions`](TextJudgeTrainOptions.md) -- the
  per-training-run counterpart, including the `dropout` override.
- [`ArchitectureRegularization`](../options/ArchitectureRegularization.md)
  -- exactly how `TextJudgeTrainOptions::architecture_regularization.dropout`
  overrides this type's own `dropout`.
