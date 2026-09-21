# `pluss::ai::classic::prep::IterativeImputerOptions`

The round count, ridge penalty, and empty-feature fallback consumed by
[`IterativeImputer::learn`](IterativeImputer.md) / `map`.

## Fields

### `rounds`

`std::int64_t rounds = 8`

How many full passes of "refit each feature's regression, re-predict its
missing rows" `learn` runs (and `map` replays with the fitted coefficients).
Must be `> 0`. More rounds let a feature's fit benefit from other features'
increasingly-refined fills within the same call, at proportionally higher
cost; the returns typically diminish well before this default.

### `ridge`

`double ridge = 1e-6`

The L2 penalty added to every regression coefficient except the intercept —
a small value purely for numerical stability against collinear or
near-degenerate features (the same "guard against numerical degeneracy
rather than meaningfully shrink" role
[`LinearFamilyOptions::regularization_strength`](../../families/LinearFamilyOptions.md#regularization_strength)'s
own tiny default plays). Must be finite and `>= 0`. Raise it only if the
per-feature regressions are visibly unstable with the default.

### `empty_feature_fallback`

`double empty_feature_fallback = 0.0`

The value a feature's fitted mean falls back to when the feature has zero
non-`NaN` training values — the same role
[`SimpleImputerOptions::empty_feature_fallback`](SimpleImputerOptions.md#empty_feature_fallback)
plays.

## Guidance

Leave `ridge` at its small default unless the regressions are visibly
unstable; it is a stability guard, not a meaningful shrinkage knob here.
`rounds` is the field worth tuning for accuracy — raise it if the batch has
several genuinely-correlated features and you want later features' fits to
benefit from earlier ones' refined values within the same `learn` call,
lower it toward `1` if fit time matters more than that refinement.

## See Also

- [`IterativeImputer`](IterativeImputer.md) — the consuming type.
- [`SimpleImputerOptions`](SimpleImputerOptions.md) — the equivalent
  fallback field on a simpler sibling imputer.
