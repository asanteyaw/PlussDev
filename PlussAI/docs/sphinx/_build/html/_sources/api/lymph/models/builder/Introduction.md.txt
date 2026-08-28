# `models::builder` Introduction

## What This Subfamily Owns

The `models::builder` subfamily owns the named candidate-model workspace
surface.

That means:

- named candidate registration
- preserving model identity under comparison
- producing a stable shortlist before spotchecking or search

It does not own fitting policy, validation policy, or deployment orchestration.
Those belong to `selection` and `workflow`.

## Main Entry

```cpp
lymph::models::Builder lymph::models::build(...);
```

The full-scope implementation lives in `lymph::models::builder::*`, but the
main-family alias `lymph::models::build(...)` is the normal user entrypoint.

## Why Users Enter Here

Users come to `builder` when one model is no longer enough and `std::vector`
or `std::map` starts becoming a weak abstraction for model comparison.

`Builder` gives that candidate set a real contract:

- stable names
- duplicate-name protection
- reusable reporting
- remembered best/worst outcomes after comparison

## Why Names Matter

`Builder` is deliberately name-first rather than position-first.

That is not cosmetic. Candidate comparison becomes much easier to read when the
workspace remembers meaningful names such as `logreg`, `tree`, or `xgb_like`
instead of asking the user to remember that candidate `2` was the tree and
candidate `5` was the nearest-neighbor baseline.

Those names travel into the reporting surfaces and into benchmark outcomes.
They become part of the experimental record, not just a convenience while
constructing the object.

## How It Cooperates With `selection`

`Builder` does not replace `selection`. It feeds it.

The normal relationship is:

1. register candidate `Model` objects in `Builder`
2. inspect the workspace with cheap listing or tabular reporting
3. benchmark or spotcheck the candidates through `selection`
4. keep the remembered best or worst fitted result as needed

That split is important because `Builder` owns candidate coordination, while
`selection` owns evidence policy and ranking logic.

## When Not To Use It

Do not use `Builder` if the project already has one clearly chosen estimator.
In that case, staying with one `lymph::models::Model` keeps the run simpler.

Also do not use `Builder` as a persistence format. It is an in-memory
candidate workspace, not the long-term deployment artifact. Once a winner has
been chosen, the practical next owner is usually `Model` or `workflow::Workflow`.

## Next Page

- [Builder](Builder.md)
