# `prep::route` Introduction

## What This Subfamily Owns

`prep::route` owns construction of reusable route objects.

That means:

- explicit name selection with `names(...)`
- predicate-driven selection with `where(...)`
- declarative complements with `remaining_of(...)`

The object these builders produce is `lymph::prep::Route`.

## Why It Exists

Without a route object, preprocessing plans drift into repeated ad-hoc column
lists and one-off predicates. That becomes difficult to audit once a project
grows beyond one scaler and one encoder.

`prep::Route` lets you construct the intent once, then resolve it repeatedly
against real frames.

## Main Entry Points

- [names](names.md)
- [where](where.md)
- [remaining_of](remaining_of.md)
- [Route](Route.md)
