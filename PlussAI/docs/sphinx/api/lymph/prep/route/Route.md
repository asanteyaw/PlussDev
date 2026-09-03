# `lymph::prep::Route`

`Route` is the reusable feature-selection object of the `prep` family: a
declarative rule for picking columns, plus a policy for what happens to the
columns it does not pick. It is produced by the
[`prep::route::*`](../route.md) builders and consumed by
[`Model::append`](../../models/Model/append.md),
[`Model::prepare`](../../models/Model/prepare.md), and
`prep::FeatureTransformer` branches.

## Scope And Alias

The type is defined at `lymph::prep::route::Route` and re-exported one scope up
as `lymph::prep::Route`. Both names are the same type; the shorter one is the
usual spelling. A `Route` is **never constructed directly** — the constructor is
private. Build one with a `lymph::prep::route::*` factory, per the
construction-versus-method rule (a `Type::factory()` spelling can never be
lower-cased into a namespace; free functions can, and that is where construction
belongs).

## Construction

| Builder | Selects |
| --- | --- |
| [`route::names({...})`](names.md) | exactly the named columns |
| [`route::where(predicate)`](where.md) | every column whose `(name, dtype)` satisfies the predicate |
| [`route::remaining_of({...})`](remaining_of.md) | every column not in the name list |
| [`route::remaining_of(other)`](remaining_of.md) | every column not claimed by `other` |

Each returns a `Route` by value. It is a small host-side object — a kind tag, a
name list or predicate, and the remainder policy — cheap to copy and to hold.

## State

`Route` carries no fitted state and no reference to any frame. Its members:

`lymph::RouteRemainderPolicy remainder = PassThrough`
: What a stage does with the columns this route did not claim: `PassThrough`
  keeps them, `Drop` removes them. Set it with
  [`with_remainder`](Route/with_remainder.md); read it back directly.

The selection rule itself (names, predicate, or the wrapped route for
`remaining_of(other)`) is private; it is observed only through
[`resolve`](Route/resolve.md).

## Methods

| Method | Summary |
| --- | --- |
| [`resolve(frame[, ordered_names])`](Route/resolve.md) | Turn the rule into concrete column positions for a given frame and column order. |
| [`with_remainder(policy)`](Route/with_remainder.md) | Set the remainder policy; returns `*this` so it chains onto a factory call. |

## Changelog

- **0.1** — `Route` unified `route()` / `route_where()` / `route_remaining()`
  plus a remainder policy into one reusable object; construction moved to the
  `prep::route::*` free functions.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Route/Introduction
Route/resolve
Route/with_remainder
```
<!-- apiref:toctree:end -->
