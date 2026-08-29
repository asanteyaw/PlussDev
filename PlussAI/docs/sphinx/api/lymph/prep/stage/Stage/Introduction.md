# `lymph::prep::Stage` Introduction

## What This Type Represents

`Stage` is the lightweight value that says what transformation should happen.

It does not decide where that transformation should apply. That is the route’s
job. The normal pair is therefore:

- one `prep::Stage`
- one `prep::Route`

bound together later through a model or feature-transformer plan.

## Why It Stays Lightweight

`Stage` is intentionally descriptor-shaped rather than execution-shaped. The
goal is to keep attachment cheap and readable while leaving actual fitting and
mapping to the broader object that owns the plan.

## Related Pages

- [contract](contract.md)
- [Notes](Notes.md)
