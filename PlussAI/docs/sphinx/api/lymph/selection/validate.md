# Lymph `selection::validate`

`validate` is the `selection` subfamily for **validation planning** — deciding
how a model is allowed to prove itself before you trust a number it produced.

A plan is a *contract*, not a one-shot call: describe the policy once
(`cross`, `stratified`, `grouped`, `time`, `holdout`), tune it with chained
setters, then run it against as many models and frames as you like.

| Entity | Role |
| --- | --- |
| [`Plan`](validate/Plan.md) | a k-fold / grouped / chronological cross-validation contract; `run` it for a report, `scores` it for a raw metric Series |
| `HoldoutPlan` | a single train/test split contract; `split` a frame into `train_set` / `test_set` |
| [`cross`](validate/cross.md) | plain k-fold CV |
| `stratified` | k-fold with class-balanced folds |
| `grouped` | k-fold that never splits a group across folds |
| [`time`](validate/time.md) | chronological (expanding-window) CV; `time_ordered` is the same |
| [`holdout`](validate/holdout.md) | one shuffled or stratified train/test split |

Options: [`options::CrossVal`](validate/CrossValOptions.md) (family alias
`selection::CrossValOptions`) drives `Plan`; `options::Holdout` drives
`HoldoutPlan`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

validate/Introduction
validate/Plan
validate/HoldoutPlan
validate/cross
validate/time
validate/holdout
```
<!-- apiref:toctree:end -->
