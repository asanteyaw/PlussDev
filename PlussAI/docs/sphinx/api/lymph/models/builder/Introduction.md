# `models::builder` Introduction

## What This Subfamily Owns

The `models::builder` subfamily owns the candidate-model workspace surface.

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

## Next Page

- [Builder](Builder.md)
