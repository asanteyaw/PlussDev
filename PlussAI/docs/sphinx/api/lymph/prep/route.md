# Lymph `prep::route`

`route` is the `prep` subfamily for **declarative feature selection** — the
"which columns" half of a preprocessing step, the piece that pairs with a
[`prep::Stage`](stage_family.md) "what".

A route is a rule, written once, that turns into concrete column positions only
when it is [`resolve`](route/Route/resolve.md)d against a real frame. Making the
selection a reusable object rather than a name list threaded through every stage
call means the intent is stated in one place, survives across cross-validation
folds and repeated calls, and is easy to compare in review.

## The three builders

All three live in the inner `lymph::prep::route` namespace and return a
[`prep::Route`](route/Route.md):

- [`route::names({...})`](route/names.md) — claim exactly the listed columns. The
  common case: explicit and stable.
- [`route::where(predicate)`](route/where.md) — claim every column whose
  `(name, dtype)` satisfies a predicate. Dynamic; follows a rule rather than a
  list.
- [`route::remaining_of({...})`](route/remaining_of.md) /
  [`route::remaining_of(other_route)`](route/remaining_of.md) — the complement:
  everything *not* claimed by a name list, or by another route.

The `Route` they produce carries two things a caller applies at the stage: the
selection itself (read with [`resolve`](route/Route/resolve.md)) and a
[remainder policy](route/Route/with_remainder.md) that says what happens to the
unclaimed columns — pass them through (the default) or drop them.

## Where it fits

```cpp
model.append(lymph::prep::stage::standard_scaler(),          // the "what"
             lymph::prep::route::names({"amount", "ratio"})); // the "where"
```

[`Model::append`](../models/Model/append.md) and
[`Model::prepare`](../models/Model/prepare.md) take a `Stage` and a `Route`
together; `prep::FeatureTransformer` uses a `Route` per branch. The route is
resolved lazily, when a frame is finally in hand.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

route/Introduction
route/Route
route/names
route/where
route/remaining_of
```
<!-- apiref:toctree:end -->
