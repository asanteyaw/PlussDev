# `pluss::ai::classic::prep::WOEEncoderOptions`

The Laplace regularization weight, vocabulary cap, and overflow policy
consumed by [`WOEEncoder::learn_supervised`](WOEEncoder.md).

## Fields

### `regularization`

`double regularization = 1.0`

The Laplace term (`reg` in the log-odds formula) added to both the positive
and negative counts before ratioing, keeping a category that is purely
positive or purely negative from producing an infinite or undefined log.
`0` disables regularization entirely — a purely one-sided category then
falls back to a fitted value of `0.0` instead (see
[`WOEEncoder`](WOEEncoder.md#methods)) rather than `±infinity`. Larger
values pull every category's WOE value closer to `0` (more conservative,
less confident evidence from small categories), the same shrinkage role
[`TargetEncoderOptions::smoothing`](TargetEncoderOptions.md#smoothing) plays
for its sibling encoder.

### `max_categories`

`std::int64_t max_categories = 4096`

The largest vocabulary `learn_supervised` will fit per feature before
`overflow` applies. `0` means unlimited. Must be `>= 0`.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

As [`OneHotEncoderOptions::overflow`](OneHotEncoderOptions.md#overflow). A
category dropped this way maps to `0.0` at `map` time, the same neutral
fallback an unseen category gets.

## Guidance

Leave `regularization` at its default unless a category is small enough
that its raw log-odds ratio would be dominated by noise — raise it for more
shrinkage toward `0`, or lower it toward `0` only once every category has
enough rows on both sides of the target that the unregularized ratio is
itself trustworthy.

## See Also

- [`WOEEncoder`](WOEEncoder.md) — the consuming type.
- [`TargetEncoderOptions`](TargetEncoderOptions.md) — the equivalent options
  for the any-target sibling encoder.
- [`OneHotEncoderOptions`](OneHotEncoderOptions.md) — the full
  `CategoryOverflowPolicy` documentation.
