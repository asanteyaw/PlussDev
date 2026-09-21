# `pluss::ai::classic::prep::TargetEncoderOptions`

The smoothing weight, vocabulary cap, and overflow policy consumed by
[`TargetEncoder::learn_supervised`](TargetEncoder.md).

## Fields

### `smoothing`

`double smoothing = 10.0`

The `m` in the m-estimate formula
`(sum_y_in_category + m * prior) / (count_in_category + m)` — effectively
how many "phantom rows at the prior" each category is blended with before
its own average target is trusted. `0` disables smoothing entirely (a
category's value becomes its raw average target, however few rows it had);
larger values pull every category's encoded value further toward `prior`,
with the pull-back strongest for categories with few rows (a category seen
`count` times moves roughly `count / (count + m)` of the way from `prior`
to its own average).

### `max_categories`

`std::int64_t max_categories = 4096`

The largest vocabulary `learn_supervised` will fit per feature before
`overflow` applies. `0` means unlimited. Must be `>= 0`.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

As [`OneHotEncoderOptions::overflow`](OneHotEncoderOptions.md#overflow). A
category dropped this way maps to `prior` at `map` time, the same fallback
an unseen category gets.

## Guidance

`smoothing` is the field worth tuning first — start near the default and
raise it if categories with very few rows appear to be overfitting the
target (their encoded value swinging to an extreme based on one or two
rows); lower it toward `0` only once you trust every category has enough
rows for its own raw average to be meaningful.

## See Also

- [`TargetEncoder`](TargetEncoder.md) — the consuming type.
- [`WOEEncoderOptions`](WOEEncoderOptions.md) — the equivalent options for
  the binary-target, log-odds-scale sibling.
- [`OneHotEncoderOptions`](OneHotEncoderOptions.md) — the full
  `CategoryOverflowPolicy` documentation.
