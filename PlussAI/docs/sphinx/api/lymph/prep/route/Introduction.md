# `prep::route` Introduction

## What This Subfamily Owns

`prep::route` owns the construction of reusable feature-selection objects. Three
builders, one product:

- [`route::names({...})`](names.md) — explicit name selection
- [`route::where(predicate)`](where.md) — predicate-driven selection over
  `(name, dtype)`
- [`route::remaining_of({...})`](remaining_of.md) — the declarative complement,
  of a name list or of another route

Every one returns a [`lymph::prep::Route`](Route.md).

## Why It Is A Separate Concern

Preprocessing plans without a route object drift into repeated ad-hoc column
lists and one-off predicates copied between stage calls — hard to audit once a
project grows past one scaler and one encoder, and easy to let a hand-maintained
"everything else" list fall out of sync with the selection it is supposed to
complement. A `Route` fixes the intent once. It is then
[resolved](Route/resolve.md) against each real frame in turn, so the same rule
serves the training frame, every cross-validation fold, and inference without
being restated.

## Main Entry Points

- [`route::names`](names.md), [`route::where`](where.md),
  [`route::remaining_of`](remaining_of.md) — build a route.
- [`Route::resolve`](Route/resolve.md) — turn it into column positions.
- [`Route::with_remainder`](Route/with_remainder.md) — set what happens to the
  unclaimed columns.
